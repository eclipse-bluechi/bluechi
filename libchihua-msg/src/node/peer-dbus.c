#include "../../include/node/peer-dbus.h"
#include "../common/dbus.h"

#include <arpa/inet.h>
#include <stdio.h>

char *assemble_address(const struct sockaddr_in *addr) {
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

PeerDBus *peer_dbus_new(const char *peer_addr, sd_event *event) {
        if (peer_addr == NULL) {
                fprintf(stderr, "Peer address must be initialized\n");
                return NULL;
        }
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
        (void) sd_bus_set_description(dbus, "orchestrator");

        /* we trust everything from the orchestrator, there is only one peer anyway */
        r = sd_bus_set_trusted(dbus, true);
        if (r < 0) {
                fprintf(stderr, "Failed to trust orchestrator: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_set_address(dbus, peer_addr);
        if (r < 0) {
                fprintf(stderr, "Failed to set address: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(dbus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return NULL;
        }

        char *dbus_addr = NULL;
        r = asprintf(&dbus_addr, "%s", peer_addr);
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }

        PeerDBus *p = malloc0(sizeof(PeerDBus));
        p->peer_dbus_addr = dbus_addr;
        p->internal_dbus = steal_pointer(&dbus);

        return p;
}

void peer_dbus_unrefp(PeerDBus **dbus) {
        fprintf(stdout, "Freeing allocated memory of PeerDBus...\n");
        if (dbus == NULL) {
                return;
        }
        if ((*dbus)->peer_dbus_addr != NULL) {
                fprintf(stdout, "Freeing allocated sd-event-source of Controller...\n");
                freep((*dbus)->peer_dbus_addr);
        }
        if ((*dbus)->internal_dbus != NULL) {
                fprintf(stdout, "Freeing allocated internal dbus of PeerDBus...\n");
                sd_bus_unrefp(&(*dbus)->internal_dbus);
        }
        free(*dbus);
}

bool peer_dbus_start(PeerDBus *dbus) {
        if (dbus == NULL) {
                return false;
        }
        return sd_bus_start(dbus->internal_dbus) >= 0;
}
