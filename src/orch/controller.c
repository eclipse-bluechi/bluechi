#include "controller.h"

#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int create_master_socket(const int port) {
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
                fprintf(stderr, "Failed to listed socket: %m\n");
                return -errsv;
        }

        return fd;
}

static int accept_handler(sd_event_source *s, int fd, uint32_t revents, void *userdata) {
        _cleanup_fd_ int nfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (nfd < 0) {
                if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK)
                        return 0;
                else {
                        int errsv = errno;
                        fprintf(stderr, "Failed to accept: %m\n");
                        return -errsv;
                }
        }

        // TODO: setup peer dbus, incl. vtable with services, signals, properties

        return 0;
}

int controller_setup(int port, sd_event *event, sd_event_source *event_source) {
        _cleanup_fd_ int accept_fd = create_master_socket(port);
        if (accept_fd < 0) {
                return -1;
        }

        int r = sd_event_add_io(event, &event_source, accept_fd, EPOLLIN, accept_handler, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %s\n", strerror(-r));
                return -1;
        }

        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %s\n", strerror(-r));
                return -1;
        }

        (void) sd_event_source_set_description(event_source, "master-socket");

        return 0;
}
