#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "../../include/common/common.h"
#include "../../include/orchestrator/connection-server.h"
#include "../../include/orchestrator/peer-manager.h"
#include "../../src/common/dbus.h"
#include "../../src/common/memory.h"

static int create_master_socket(uint16_t port) {
        struct sockaddr_in servaddr;

        int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0) {
                int errsv = errno;
                fprintf(stderr, "Failed to create socket: %m\n");
                return -errsv;
        }

        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                int errsv = errno;
                fprintf(stderr, "Failed to create socket: %m\n");
                return -errsv;
        }

        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(port);

        if (bind(fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
                int errsv = errno;
                fprintf(stderr, "Failed to bind socket: %m\n");
                return -errsv;
        }

        if ((listen(fd, SOMAXCONN)) != 0) {
                int errsv = errno;
                fprintf(stderr, "Failed to listen to socket: %m\n");
                return -errsv;
        }

        return fd;
}


static int accept_connection_handler(
                UNUSED sd_event_source *source, int fd, UNUSED uint32_t revents, void *userdata) {
        _cleanup_fd_ int nfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (nfd < 0) {
                if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                        return 0;
                }

                int errsv = errno;
                fprintf(stderr, "Failed to accept: %m\n");
                return -errsv;
        }

        bool successful = peer_manager_accept_connection_request((PeerManager *) userdata, nfd);
        if (!successful) {
                return -1;
        }
        // peer dbus will take care of closing fd, setting to -1 prevents closing it twice
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        nfd = -1;

        return 0;
}

ConnectionServer *connection_server_new(uint16_t port, sd_event *event, PeerManager *mgr) {
        fprintf(stdout, "Creating ConnectionServer...\n");

        int r = 0;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        _cleanup_fd_ int accept_fd = create_master_socket(port);
        if (accept_fd < 0) {
                return NULL;
        }

        r = sd_event_add_io(event, &event_source, accept_fd, EPOLLIN, accept_connection_handler, mgr);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %s\n", strerror(-r));
                return NULL;
        }
        // sd_event_add_io takes care of closing accept_fd, setting it to -1 avoids closing it multiple times
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        accept_fd = -1;

        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %s\n", strerror(-r));
                return NULL;
        }

        (void) sd_event_source_set_description(event_source, "master-socket");

        ConnectionServer *c = malloc0(sizeof(ConnectionServer));
        c->sd_event_source = steal_pointer(&event_source);
        c->peer_manager = steal_pointer(&mgr);

        return c;
}

void connection_server_unrefp(ConnectionServer **server) {
        fprintf(stdout, "Freeing allocated memory of ConnectionServer...\n");
        if (server == NULL || (*server) == NULL) {
                return;
        }
        if ((*server)->sd_event_source != NULL) {
                fprintf(stdout, "Freeing allocated sd-event-source of ConnectionServer...\n");
                sd_event_source_unrefp(&(*server)->sd_event_source);
        }
        free(*server);
}
