#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <systemd/sd-event.h>

#include "./common/memory.h"
#include "./orchestrator/peer-manager.h"

#define ORCHESTRATOR_SERVICE_DEFAULT_NAME "org.containers.hirte.Orchestrator"

typedef struct {
        uint16_t accept_port;
        char *user_bus_service_name;

        sd_event *event;
        sd_event_source *peer_connection_source;

        sd_bus *user_dbus;

        PeerManager *peer_manager;
} Orchestrator;

Orchestrator *orch_new(uint16_t port, const char *bus_service_name);
void orch_unrefp(Orchestrator **orchestrator);

bool orch_start(const Orchestrator *orchestrator);
bool orch_stop(const Orchestrator *orchestrator);

#define _cleanup_orchestrator_ _cleanup_(orch_unrefp)
