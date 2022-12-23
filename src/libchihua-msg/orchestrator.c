#include "orchestrator.h"
#include "../common/memory.h"

#include <stdio.h>

Orchestrator *orch_new(OrchestratorParams *p) {
        fprintf(stdout, "Creating Orchestrator...\n");
        Orchestrator *o = malloc0(sizeof(Orchestrator));
        return o;
}

void orch_start(Orchestrator *o) {
        fprintf(stdout, "Starting Orchestrator...\n");
}

void orch_stop(Orchestrator *o) {
        fprintf(stdout, "Stopping Orchestrator...\n");
}
