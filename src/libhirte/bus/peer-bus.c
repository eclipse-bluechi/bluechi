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

static sd_bus *peer_bus_new(sd_event *event, char *dbus_description) {
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

        r = sd_bus_attach_event(dbus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}

sd_bus *peer_bus_open(sd_event *event, char *dbus_description, char *dbus_server_addr) {
        if (dbus_server_addr == NULL) {
                fprintf(stderr, "Peer address to connect to must be initialized\n");
                return NULL;
        }

        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = peer_bus_new(event, dbus_description);
        if (dbus == NULL) {
                return NULL;
        }
        // trust everything from the orchestrator
        r = sd_bus_set_trusted(dbus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to trust orchestrator: %s\n", strerror(-r));
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

        return steal_pointer(&dbus);
}

/* Takes ownership of fd */
sd_bus *peer_bus_open_server(sd_event *event, char *dbus_description, int fd) {
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
                return 0;
        }

        r = sd_bus_start(dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to start peer bus server: %s", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}
