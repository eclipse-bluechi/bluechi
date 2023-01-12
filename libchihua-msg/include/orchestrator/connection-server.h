#pragma once

#include <stdbool.h>
#include <systemd/sd-event.h>

#include "../common/list.h"
#include "../common/memory.h"
#include "peer-manager.h"

typedef struct {
        sd_event_source *sd_event_source;

        PeerManager *peer_manager;
} ConnectionServer;

ConnectionServer *connection_server_new(uint16_t port, sd_event *event, PeerManager *mgr);
void connection_server_unrefp(ConnectionServer **server);

#define _cleanup_connection_server_ _cleanup_(connection_server_unrefp)
