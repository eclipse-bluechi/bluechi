#include <errno.h>
#include <stdio.h>

#include "../../include/orchestrator/peer-manager.h"
#include "../../src/common/dbus.h"
#include "../../src/common/memory.h"

PeerManager *peer_manager_new(sd_event *event_loop) {
        fprintf(stdout, "Creating Peer Manager...\n");

        PeerManager *mgr = malloc0(sizeof(PeerManager));
        mgr->event_loop = event_loop;
        LIST_HEAD_INIT(mgr->peers_head);

        return mgr;
}

void peer_manager_unrefp(PeerManager **manager) {
        if (manager == NULL || (*manager) == NULL) {
                return;
        }

        free(*manager);
}

bool peer_manager_accept_connection_request(PeerManager *mgr, int fd) {
        _cleanup_sd_bus_ sd_bus *dbus_server = peer_bus_open_server(mgr->event_loop, "peer-to-node", fd);
        if (dbus_server == NULL) {
                return false;
        }

        PeerBusListItem *item = peer_bus_list_item_new(dbus_server);
        LIST_APPEND(peers, mgr->peers_head, item);

        return true;
}