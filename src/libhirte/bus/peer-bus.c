#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>

#include "libhirte/common/common.h"
#include "peer-bus.h"

char *assemble_tcp_address(const struct sockaddr_in *addr) {
        if (addr == NULL) {
                fprintf(stderr, "Can not assemble an empty address\n");
                return NULL;
        }

        char *dbus_addr = NULL;
        int r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }
        return dbus_addr;
}

static sd_bus *peer_bus_new(sd_event *event, const char *dbus_description) {
        if (event == NULL) {
                fprintf(stderr, "Event loop must be initialized\n");
                return NULL;
        }

        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = NULL;

        r = sd_bus_new(&dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to create bus: %s\n", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(dbus, dbus_description);

        // trust everything to/from the orchestrator
        r = sd_bus_set_trusted(dbus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to trust orchestrator: %s\n", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}

sd_bus *peer_bus_open(sd_event *event, const char *dbus_description, const char *dbus_server_addr) {
        if (dbus_server_addr == NULL) {
                fprintf(stderr, "Peer address to connect to must be initialized\n");
                return NULL;
        }

        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = peer_bus_new(event, dbus_description);
        if (dbus == NULL) {
                return NULL;
        }
        r = sd_bus_set_address(dbus, dbus_server_addr);
        if (r < 0) {
                fprintf(stderr, "Failed to set address: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_start(dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to start peer bus: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(dbus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}

/* This is just some helper to make the peer connections look like a bus for e.g busctl */

static int method_peer_hello(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        /* Reply with the response */
        return sd_bus_reply_method_return(m, "s", ":1.0");
}

static const sd_bus_vtable peer_bus_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Hello", "", "s", method_peer_hello, SD_BUS_VTABLE_UNPRIVILEGED),
        SD_BUS_VTABLE_END
};

/* Takes ownership of fd */
sd_bus *peer_bus_open_server(sd_event *event, const char *dbus_description, const char *sender, int fd) {
        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = peer_bus_new(event, dbus_description);
        if (dbus == NULL) {
                close(fd);
                return NULL;
        }

        r = sd_bus_set_fd(dbus, fd, fd);
        if (r < 0) {
                fprintf(stderr, "Failed to set fd on new connection bus: %s\n", strerror(-r));
                return NULL;
        }
        sd_id128_t server_id;
        r = sd_id128_randomize(&server_id);
        if (r < 0) {
                fprintf(stderr, "Failed to create server id: %s\n", strerror(-r));
        }
        r = sd_bus_set_server(dbus, 1, server_id);
        if (r < 0) {
                fprintf(stderr, "Failed to enable server support for new connection bus: %s\n", strerror(-r));
                return NULL;
        }
        r = sd_bus_negotiate_creds(
                        dbus,
                        1,
                        // NOLINTNEXTLINE
                        SD_BUS_CREDS_PID | SD_BUS_CREDS_UID | SD_BUS_CREDS_EUID |
                                        SD_BUS_CREDS_EFFECTIVE_CAPS | SD_BUS_CREDS_SELINUX_CONTEXT);
        if (r < 0) {
                fprintf(stderr, "Failed to enable credentials for new connection: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_set_anonymous(dbus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set bus to anonymous: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_set_sender(dbus, sender);
        if (r < 0) {
                fprintf(stderr, "Failed to set sender for new connection bus: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_start(dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to start peer bus server: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_add_object_vtable(
                        dbus, NULL, "/org/freedesktop/DBus", "org.freedesktop.DBus", peer_bus_vtable, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(dbus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}
