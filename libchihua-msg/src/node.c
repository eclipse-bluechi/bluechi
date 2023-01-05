#include "../include/node.h"
#include "../include/node/peer-dbus.h"
#include "./common/dbus.h"

#include <stdio.h>

Node *node_new(const NodeParams *p) {
        fprintf(stdout, "Creating Node...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_new(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        char *orch_addr = assemble_address(p->orch_addr);
        if (orch_addr == NULL) {
                return NULL;
        }

        Node *n = malloc0(sizeof(Node));
        n->event_loop = steal_pointer(&event);
        n->orch_addr = orch_addr;

        return n;
}

void node_unrefp(Node **n) {
        fprintf(stdout, "Freeing allocated memory of Orchestrator...\n");
        if (n == NULL) {
                return;
        }
        if ((*n)->event_loop != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Orchestrator...\n");
                sd_event_unrefp(&(*n)->event_loop);
        }
        if ((*n)->orch_addr != NULL) {
                fprintf(stdout, "Freeing allocated orch_addr of Orchestrator...\n");
                freep((*n)->orch_addr);
        }

        free(*n);
}

bool node_start(const Node *n) {
        fprintf(stdout, "Starting Node...\n");

        if (n == NULL) {
                return false;
        }

        _cleanup_peer_dbus_ PeerDBus *orch_dbus = peer_dbus_new(n->orch_addr, n->event_loop);
        if (orch_dbus == NULL) {
                return false;
        }
        bool started_successful = peer_dbus_start(orch_dbus);
        if (!started_successful) {
                return false;
        }

        int r = 0;
        r = sd_event_loop(n->event_loop);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool node_stop(const Node *n) {
        fprintf(stdout, "Stopping Node...\n");

        if (n == NULL) {
                return false;
        }

        return true;
}
