#include "dbus.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>

int setup_peer_dbus(sd_bus *dbus, const struct sockaddr_in *addr) {
        int r = 0;

        r = sd_bus_new(&dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to create bus: %s\n", strerror(-r));
                return -1;
        }
        (void) sd_bus_set_description(dbus, "orchestrator");

        /* we trust everything from the orchestrator, there is only one peer anyway */
        r = sd_bus_set_trusted(dbus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to trust orchestrator: %s\n", strerror(-r));
                return -1;
        }

        _cleanup_free_ char *dbus_addr = NULL;
        r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return -1;
        }

        r = sd_bus_set_address(dbus, dbus_addr);
        if (r < 0) {
                fprintf(stderr, "Failed to set address: %s\n", strerror(-r));
                return -1;
        }

        fprintf(stdout, "connecting to orchestrator on '%s'\n", dbus_addr);
        r = sd_bus_start(dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to orchestrator: %s\n", strerror(-r));
                return -1;
        }

        return 0;
}