#ifndef _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR
#define _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR

#include "./common/memory.h"

#include <inttypes.h>
#include <stdbool.h>
#include <systemd/sd-event.h>

typedef struct {
        uint16_t port;
} OrchestratorParams;

typedef struct {
        uint16_t accept_port;
        sd_event *event_loop;
} Orchestrator;

Orchestrator *orch_new(const OrchestratorParams *params);
void orch_unrefp(Orchestrator **orchestrator);

bool orch_start(const Orchestrator *orchestrator);
bool orch_stop(const Orchestrator *orchestrator);

#define _cleanup_orchestrator_ _cleanup_(orch_unrefp)

#endif