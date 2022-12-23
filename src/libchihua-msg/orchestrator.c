#include "orchestrator.h"

#include <stdio.h>

Orchestrator *orch_new(OrchestratorParams *p) {
        fprintf(stdout, "Creating Orchestrator...\n");

        int r = 0;
        sd_event *event;
        r = sd_event_new(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        Orchestrator *o = malloc0(sizeof(Orchestrator));
        o->event_loop = steal_pointer(&event);

        return o;
}

void orch_unrefp(Orchestrator **o) {
        fprintf(stdout, "Freeing allocated memory of Orchestrator...\n");
        if (o == NULL) {
                return;
        }
        if ((*o)->event_loop != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Orchestrator...\n");
                sd_event_unrefp(&(*o)->event_loop);
        }
        free(*o);
}

bool orch_start(Orchestrator *o) {
        fprintf(stdout, "Starting Orchestrator...\n");

        if (o == NULL) {
                return false;
        }

        int r = 0;
        r = sd_event_loop(o->event_loop);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool orch_stop(Orchestrator *o) {
        fprintf(stdout, "Stopping Orchestrator...\n");
        return true;
}
