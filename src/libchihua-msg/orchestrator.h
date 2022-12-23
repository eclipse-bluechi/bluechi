#ifndef _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR
#define _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR

#include "../common/memory.h"

typedef struct {

} OrchestratorParams;

typedef struct {

} Orchestrator;

Orchestrator *orch_new(OrchestratorParams *p);
void orch_unrefp(Orchestrator **o);

void orch_start(Orchestrator *o);
void orch_stop(Orchestrator *o);

#define _cleanup_orchestrator_ _cleanup_(orch_unrefp)

#endif