#include "peer-dbus.h"

#include <arpa/inet.h>
#include <stdio.h>

char *assemble_address(const struct sockaddr_in *addr) {
        char *dbus_addr = NULL;
        int r = asprintf(&dbus_addr, "tcp:host=%s,port=%d", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }
        return dbus_addr;
}

sd_bus *peer_dbus_new(const char *dbus_addr) {
        int r = 0;
        _cleanup_sd_bus_ sd_bus *dbus = NULL;

        r = sd_bus_new(&dbus);
        if (r < 0) {
                fprintf(stderr, "Failed to create bus: %s\n", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(dbus, "orchestrator");

        /* we trust everything from the orchestrator, there is only one peer anyway */
        r = sd_bus_set_trusted(dbus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to trust orchestrator: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_set_address(dbus, dbus_addr);
        if (r < 0) {
                fprintf(stderr, "Failed to set address: %s\n", strerror(-r));
                return NULL;
        }

        return steal_pointer(&dbus);
}

bool peer_dbus_start(sd_bus *dbus) {
        return sd_bus_start(dbus) >= 0;
}
