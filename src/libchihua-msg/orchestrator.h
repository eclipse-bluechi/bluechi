#ifndef _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR
#define _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR

typedef struct {

} OrchestratorParams;

typedef struct {

} Orchestrator;

Orchestrator *orch_new(OrchestratorParams *p);
void orch_start(Orchestrator *o);
void orch_stop(Orchestrator *o);

#endif