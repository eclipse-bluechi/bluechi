#pragma once

#include "../../include/common/common.h"
#include "../../include/common/list.h"
#include "../peer-bus.h"

typedef struct {
        sd_event *event_loop;

        LIST_HEAD(PeerBusListItem, peers_head);
} PeerManager;

PeerManager *peer_manager_new(sd_event *event_loop);
void peer_manager_unrefp(PeerManager **manager);

int peer_manager_accept_connection_request(
                UNUSED sd_event_source *source, int fd, UNUSED uint32_t revents, void *userdata);

#define _cleanup_peer_manager_ _cleanup_(peer_manager_unrefp)