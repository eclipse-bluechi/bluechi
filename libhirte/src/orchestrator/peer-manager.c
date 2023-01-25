#include <errno.h>
#include <stdio.h>

#include "../../include/orchestrator/peer-manager.h"
#include "../../include/socket.h"
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

int peer_manager_accept_connection_request(
                UNUSED sd_event_source *source, int fd, UNUSED uint32_t revents, void *userdata) {
        _cleanup_fd_ int nfd = accept_tcp_connection_request(fd);

        PeerManager *mgr = (PeerManager *) userdata;

        _cleanup_sd_bus_ sd_bus *dbus_server = peer_bus_open_server(mgr->event_loop, "peer-to-node", nfd);
        if (dbus_server == NULL) {
                return -1;
        }
        // peer dbus will take care of closing fd, setting to -1 prevents closing it twice
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        nfd = -1;

        PeerBusListItem *item = peer_bus_list_item_new(dbus_server);
        LIST_APPEND(peers, mgr->peers_head, item);

        printf("Peer connection accepted...\n");

        return 0;
}