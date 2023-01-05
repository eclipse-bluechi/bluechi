#include "../include/orchestrator.h"
#include "../include/orchestrator/controller.h"
#include "./common/dbus.h"

#include <stdio.h>
#include <string.h>

Orchestrator *orch_new(const OrchestratorParams *params) {
        fprintf(stdout, "Creating Orchestrator...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        Orchestrator *o = malloc0(sizeof(Orchestrator));
        o->event_loop = steal_pointer(&event);
        o->accept_port = params->port;

        return o;
}

void orch_unrefp(Orchestrator **orchestrator) {
        fprintf(stdout, "Freeing allocated memory of Orchestrator...\n");
        if (orchestrator == NULL) {
                return;
        }
        if ((*orchestrator)->event_loop != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Orchestrator...\n");
                sd_event_unrefp(&(*orchestrator)->event_loop);
        }
        free(*orchestrator);
}

bool orch_start(const Orchestrator *orchestrator) {
        fprintf(stdout, "Starting Orchestrator...\n");

        if (orchestrator == NULL) {
                return false;
        }
        if (orchestrator->event_loop == NULL) {
                return false;
        }

        _cleanup_controller_ Controller *c = NULL;
        // NOLINTNEXTLINE
        c = controller_new(orchestrator->accept_port, orchestrator->event_loop, default_accept_handler());

        int r = 0;
        r = sd_event_loop(orchestrator->event_loop);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool orch_stop(const Orchestrator *orchestrator) {
        fprintf(stdout, "Stopping Orchestrator...\n");

        if (orchestrator == NULL) {
                return false;
        }

        return true;
}
