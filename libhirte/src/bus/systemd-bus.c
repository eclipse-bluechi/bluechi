#include <errno.h>
#include <stdio.h>

#include "../../include/bus/systemd-bus.h"
#include "../../include/socket.h"
#include "../common/dbus.h"

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
                fprintf(stderr, "Failed to connect to systemd bus: %s\n", strerror(-r));
                return NULL;
        }
        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach systemd bus to event: %s\n", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(bus, "systemd-bus");

        return steal_pointer(&bus);
}
