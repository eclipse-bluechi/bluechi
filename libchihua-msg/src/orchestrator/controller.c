#include "../../include/orchestrator/controller.h"
#include "../common/dbus.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

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

Controller *controller_new(uint16_t port, sd_event *event, sd_event_io_handler_t accept_handler) {
        fprintf(stdout, "Creating Controller...\n");

        int r = 0;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        int accept_fd = create_master_socket(port);
        if (accept_fd < 0) {
                return NULL;
        }

        r = sd_event_add_io(event, &event_source, accept_fd, EPOLLIN, accept_handler, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %s\n", strerror(-r));
                return NULL;
        }

        (void) sd_event_source_set_description(event_source, "master-socket");

        Controller *c = malloc0(sizeof(Controller));
        c->accept_fd = accept_fd;
        c->sd_event_source = steal_pointer(&event_source);

        return c;
}

void controller_unrefp(Controller **c) {
        fprintf(stdout, "Freeing allocated memory of Controller...\n");
        if (c == NULL) {
                return;
        }
        if ((*c)->sd_event_source != NULL) {
                fprintf(stdout, "Freeing allocated sd-event-source of Controller...\n");
                sd_event_source_unrefp(&(*c)->sd_event_source);
        }
        if ((*c)->accept_fd >= 0) {
                closep(&(*c)->accept_fd);
        }
        free(*c);
}

// NOLINTNEXTLINE
static int accept_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        _cleanup_fd_ int nfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (nfd < 0) {
                if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                        return 0;
                }

                int errsv = errno;
                fprintf(stderr, "Failed to accept: %m\n");
                return -errsv;
        }

        // TODO: setup peer dbus, incl. vtable with services, signals, properties
        fprintf(stdout, "accepted connection request\n");

        return 0;
}

sd_event_io_handler_t default_accept_handler() {
        return &accept_handler;
}