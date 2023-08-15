/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <arpa/inet.h>
#include <errno.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/network.h"
#include "libbluechi/log/log.h"
#include "libbluechi/socket.h"

#include "bus.h"

/*****************
 *** peer dbus ***
 *****************/

static sd_bus *peer_bus_new(sd_event *event, const char *dbus_description) {
        if (event == NULL) {
                bc_log_error("Event loop must be initialized");
                return NULL;
        }

        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = NULL;

        r = sd_bus_new(&dbus);
        if (r < 0) {
                bc_log_errorf("Failed to create bus: %s", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(dbus, dbus_description);

        // trust everything to/from the controller
        r = sd_bus_set_trusted(dbus, true);
        if (r < 0) {
                bc_log_errorf("Failed to trust controller: %s", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}

sd_bus *peer_bus_open(sd_event *event, const char *dbus_description, const char *dbus_server_addr) {
        if (dbus_server_addr == NULL) {
                bc_log_error("Peer address to connect to must be initialized");
                return NULL;
        }

        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = peer_bus_new(event, dbus_description);
        if (dbus == NULL) {
                return NULL;
        }
        r = sd_bus_set_address(dbus, dbus_server_addr);
        if (r < 0) {
                bc_log_errorf("Failed to set address: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_start(dbus);
        if (r < 0) {
                bc_log_errorf("Failed to start peer bus: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(dbus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                bc_log_errorf("Failed to attach bus to event: %s", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}

int peer_bus_close(sd_bus *peer_dbus) {
        if (peer_dbus != NULL) {
                int r = sd_bus_detach_event(peer_dbus);
                if (r < 0) {
                        bc_log_errorf("Failed to detach bus from event: %s", strerror(-r));
                        return r;
                }

                sd_bus_flush_close_unref(peer_dbus);
        }

        return 0;
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
                bc_log_errorf("Failed to set fd on new connection bus: %s", strerror(-r));
                return NULL;
        }
        sd_id128_t server_id;
        r = sd_id128_randomize(&server_id);
        if (r < 0) {
                bc_log_errorf("Failed to create server id: %s", strerror(-r));
        }
        r = sd_bus_set_server(dbus, 1, server_id);
        if (r < 0) {
                bc_log_errorf("Failed to enable server support for new connection bus: %s", strerror(-r));
                return NULL;
        }
        r = sd_bus_negotiate_creds(
                        dbus,
                        1,
                        // NOLINTNEXTLINE
                        SD_BUS_CREDS_PID | SD_BUS_CREDS_UID | SD_BUS_CREDS_EUID |
                                        SD_BUS_CREDS_EFFECTIVE_CAPS | SD_BUS_CREDS_SELINUX_CONTEXT);
        if (r < 0) {
                bc_log_errorf("Failed to enable credentials for new connection: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_set_anonymous(dbus, true);
        if (r < 0) {
                bc_log_errorf("Failed to set bus to anonymous: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_set_sender(dbus, sender);
        if (r < 0) {
                bc_log_errorf("Failed to set sender for new connection bus: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_start(dbus);
        if (r < 0) {
                bc_log_errorf("Failed to start peer bus server: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_add_object_vtable(
                        dbus, NULL, "/org/freedesktop/DBus", "org.freedesktop.DBus", peer_bus_vtable, NULL);
        if (r < 0) {
                bc_log_errorf("Failed to add peer bus vtable: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(dbus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                bc_log_errorf("Failed to attach bus to event: %s", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}


/*******************
 *** system dbus ***
 *******************/

sd_bus *system_bus_open(sd_event *event) {
        int r = 0;
        _cleanup_sd_bus_ sd_bus *bus = NULL;

        r = sd_bus_open_system(&bus);
        if (r < 0) {
                bc_log_errorf("Failed to connect to system bus: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                bc_log_errorf("Failed to attach bus to event: %s", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(bus, "system-bus");

        bc_log_debug("Connected to system bus");

        return steal_pointer(&bus);
}


/********************
 *** systemd dbus ***
 ********************/

static int systemd_bus_new(sd_bus **ret) {
        if (ret == NULL) {
                return -EINVAL;
        }

        int r = 0;
        _cleanup_sd_bus_ sd_bus *bus = NULL;

        if (geteuid() != 0) {
                r = sd_bus_default_system(&bus);
                if (r < 0) {
                        return r;
                }
                *ret = steal_pointer(&bus);
                return 1;
        }

        r = sd_bus_new(&bus);
        if (r < 0) {
                return r;
        }
        r = sd_bus_set_address(bus, "unix:path=/run/systemd/private");
        if (r < 0) {
                return r;
        }
        r = sd_bus_start(bus);
        if (r < 0) {
                return r;
        }

        int fd = sd_bus_get_fd(bus);
        if (fd < 0) {
                return fd;
        }
        r = fd_check_peercred(fd);
        if (r < 0) {
                return r;
        }

        *ret = steal_pointer(&bus);
        return 1;
}

sd_bus *systemd_bus_open(sd_event *event) {
        int r = 0;
        _cleanup_sd_bus_ sd_bus *bus = NULL;

        r = systemd_bus_new(&bus);
        if (r < 0) {
                bc_log_errorf("Failed to connect to systemd bus: %s", strerror(-r));
                return NULL;
        }
        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                bc_log_errorf("Failed to attach systemd bus to event: %s", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(bus, "systemd-bus");

        bc_log_debug("Connected to systemd bus");

        return steal_pointer(&bus);
}


/*****************
 *** user dbus ***
 *****************/

sd_bus *user_bus_open(sd_event *event) {
        int r = 0;
        _cleanup_sd_bus_ sd_bus *bus = NULL;

        r = sd_bus_open_user(&bus);
        if (r < 0) {
                bc_log_errorf("Failed to connect to user bus: %s", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                bc_log_errorf("Failed to attach bus to event: %s", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(bus, "user-bus");

        bc_log_debug("Connected to user bus");

        return steal_pointer(&bus);
}
