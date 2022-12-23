#include "node.h"

#include <stdio.h>

Node *node_new(NodeParams *p) {
        fprintf(stdout, "Creating Node...\n");

        int r = 0;
        sd_event *event;
        r = sd_event_new(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        Node *n = malloc0(sizeof(Node));
        n->event_loop = steal_pointer(&event);

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
        free(*n);
}

bool node_start(Node *n) {
        fprintf(stdout, "Starting Node...\n");

        if (n == NULL) {
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

bool node_stop(Node *n) {
        fprintf(stdout, "Stopping Node...\n");
        return true;
}
