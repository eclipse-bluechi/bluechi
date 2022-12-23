#ifndef _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR
#define _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR

#include "../common/dbus.h"
#include "../common/memory.h"

#include <inttypes.h>
#include <stdbool.h>

typedef struct {
        uint16_t port;
} OrchestratorParams;

typedef struct {
        sd_event *event_loop;
} Orchestrator;

Orchestrator *orch_new(OrchestratorParams *p);
void orch_unrefp(Orchestrator **o);

bool orch_start(Orchestrator *o);
bool orch_stop(Orchestrator *o);

#define _cleanup_orchestrator_ _cleanup_(orch_unrefp)

#endif