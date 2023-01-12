#pragma once

#include "../../include/common/list.h"
#include "../peer-bus.h"

typedef struct {
        sd_event *event_loop;

        LIST_HEAD(PeerBusListItem, peers_head);
} PeerManager;

PeerManager *peer_manager_new(sd_event *event_loop);
void peer_manager_unrefp(PeerManager **manager);

bool peer_manager_accept_connection_request(PeerManager *mgr, int fd);

#define _cleanup_peer_manager_ _cleanup_(peer_manager_unrefp)