#include "../include/orchestrator.h"
#include "../include/orchestrator/controller.h"
#include "./common/memory.h"

#include <stdio.h>
#include <string.h>

Orchestrator *orch_new(const OrchestratorParams *p) {
        fprintf(stdout, "Creating Orchestrator...\n");

        int r = 0;
        sd_event *event = NULL;
        r = sd_event_new(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        Orchestrator *o = malloc0(sizeof(Orchestrator));
        o->event_loop = steal_pointer(&event);
        o->accept_port = p->port;

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

bool orch_start(const Orchestrator *o) {
        fprintf(stdout, "Starting Orchestrator...\n");

        if (o == NULL) {
                return false;
        }
        if (o->event_loop == NULL) {
                return false;
        }

        _cleanup_controller_ Controller *c = NULL;
        // NOLINTNEXTLINE
        c = controller_new(o->accept_port, o->event_loop, default_accept_handler());

        int r = 0;
        r = sd_event_loop(o->event_loop);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool orch_stop(const Orchestrator *o) {
        fprintf(stdout, "Stopping Orchestrator...\n");

        if (o == NULL) {
                return false;
        }

        return true;
}
