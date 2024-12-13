/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/log/log.h"
#include "socket.h"

int create_tcp_socket(uint16_t port) {
        struct sockaddr_in6 servaddr;

        _cleanup_fd_ int fd = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0) {
                int errsv = errno;
                return -errsv;
        }

        int yes = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
                int errsv = errno;
                return -errsv;
        }

        servaddr.sin6_family = AF_INET6;
        servaddr.sin6_addr = in6addr_any;
        servaddr.sin6_port = htons(port);
        servaddr.sin6_flowinfo = 0;
        servaddr.sin6_scope_id = 0;

        if (bind(fd, &servaddr, sizeof(servaddr)) < 0) {
                int errsv = errno;
                return -errsv;
        }

        if ((listen(fd, SOMAXCONN)) != 0) {
                int errsv = errno;
                return -errsv;
        }

        return steal_fd(&fd);
}


int create_uds_socket(const char *path) {
        struct sockaddr_un servaddr;

        _cleanup_fd_ int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
                int errsv = errno;
                return -errsv;
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sun_family = AF_UNIX;
        strncpy(servaddr.sun_path, path, sizeof(servaddr.sun_path));
        servaddr.sun_path[sizeof(servaddr.sun_path) - 1] = '\0';

        if (bind(fd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
                int errsv = errno;
                return -errsv;
        }
        if ((listen(fd, SOMAXCONN)) != 0) {
                int errsv = errno;
                return -errsv;
        }

        return steal_fd(&fd);
}

int accept_connection_request(int fd) {
        int nfd = accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (nfd < 0) {
                if (errno == EAGAIN || errno == EINTR || errno == EWOULDBLOCK) {
                        return 0;
                }

                int errsv = errno;
                return -errsv;
        }
        return nfd;
}

int fd_check_peercred(int fd) {
        int r = 0;
        struct ucred ucred = { .pid = -1, .uid = -1, .gid = -1 };
        socklen_t n = sizeof(struct ucred);

        r = getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &ucred, &n);
        if (r < 0) {
                return -errno;
        }
        if (n != sizeof(struct ucred)) {
                return -EIO;
        }
        // Check if the data is actually useful and not suppressed due to namespacing issues
        if (ucred.pid <= 0) {
                return -ENODATA;
        }
        if (ucred.uid != 0 && ucred.uid != geteuid()) {
                return -EPERM;
        }

        /* Note:
           We don't check UID/GID here as namespace translation works differently in this case.
           Instead of receiving an "invalid" user/group we get the overflow UID/GID.
        */

        return 1;
}


/*
 * SocketOptions
 */
struct SocketOptions {
        int tcp_keepidle;
        int tcp_keepintvl;
        int tcp_keepcnt;
        bool ip_recv_err;
};

SocketOptions *socket_options_new() {
        SocketOptions *opts = malloc0(sizeof(SocketOptions));
        if (opts == NULL) {
                return NULL;
        }
        opts->tcp_keepidle = 0;
        opts->tcp_keepintvl = 0;
        opts->tcp_keepcnt = 0;
        opts->ip_recv_err = false;
        return opts;
}

/*
 * TCP max values for idle, interval and count based on
 * https://github.com/torvalds/linux/blob/master/include/net/tcp.h
 */
#define MAX_TCP_KEEPIDLE 32767
#define MAX_TCP_KEEPINTVL 32767
#define MAX_TCP_KEEPCNT 127

int socket_options_set_tcp_keepidle(SocketOptions *opts, const char *keepidle_s) {
        if (opts == NULL) {
                return -EINVAL;
        }

        long keepidle = 0;
        if (!parse_long(keepidle_s, &keepidle)) {
                return -EINVAL;
        }
        if (keepidle < 0 || keepidle > MAX_TCP_KEEPIDLE) {
                return -EINVAL;
        }
        opts->tcp_keepidle = (int) keepidle;

        return 0;
}

int socket_options_set_tcp_keepintvl(SocketOptions *opts, const char *keepintvl_s) {
        if (opts == NULL) {
                return -EINVAL;
        }

        long keepintvl = 0;
        if (!parse_long(keepintvl_s, &keepintvl)) {
                return -EINVAL;
        }
        if (keepintvl < 0 || keepintvl > MAX_TCP_KEEPINTVL) {
                return -EINVAL;
        }
        opts->tcp_keepintvl = (int) keepintvl;

        return 0;
}

int socket_options_set_tcp_keepcnt(SocketOptions *opts, const char *keepcnt_s) {
        if (opts == NULL) {
                return -EINVAL;
        }

        long keepcnt = 0;
        if (!parse_long(keepcnt_s, &keepcnt)) {
                return -EINVAL;
        }
        if (keepcnt < 0 || keepcnt > MAX_TCP_KEEPCNT) {
                return -EINVAL;
        }
        opts->tcp_keepcnt = (int) keepcnt;

        return 0;
}

int socket_options_set_ip_recverr(SocketOptions *opts, bool recverr) {
        if (opts == NULL) {
                return -EINVAL;
        }

        opts->ip_recv_err = recverr;
        return 0;
}

bool fd_is_socket_tcp(int fd) {
        int type = 0;
        socklen_t length = sizeof(int);

        getsockopt(fd, SOL_SOCKET, SO_DOMAIN, &type, &length);

        return type == AF_INET || type == AF_INET6;
}

int socket_set_options(int fd, SocketOptions *opts) {
        /* Just exit in case of a non-tcp socket. We could connect to a UNIX socket as well. */
        if (!fd_is_socket_tcp(fd)) {
                bc_log_debug("Socket options not set. FD is a not TCP socket.");
                return 0;
        }

        if (opts == NULL) {
                bc_log_error("Socket options are NULL.");
                return -EINVAL;
        }

        int r = 0;
        int enable_flag = 1;

        if (opts->ip_recv_err) {
                r = setsockopt(fd, IPPROTO_IP, IP_RECVERR, &enable_flag, sizeof(int));
                if (r < 0) {
                        bc_log_errorf("Failed to set IP_RECVERR: %s", strerror(errno));
                        return -errno;
                }
        }

        r = setsockopt(fd, SOL_TCP, TCP_NODELAY, (char *) &enable_flag, sizeof(int));
        if (r < 0) {
                bc_log_errorf("Failed to set TCP_NODELAY: %s", strerror(errno));
                return -errno;
        }

        r = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &enable_flag, sizeof(int));
        if (r < 0) {
                bc_log_errorf("Failed to set SO_KEEPALIVE: %s", strerror(errno));
                return -errno;
        }

        if (opts->tcp_keepidle != 0) {
                r = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &opts->tcp_keepidle, sizeof(int));
                if (r < 0) {
                        bc_log_errorf("Failed to set TCP_KEEPIDLE: %s", strerror(errno));
                        return -errno;
                }
        }
        if (opts->tcp_keepintvl != 0) {
                r = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &opts->tcp_keepintvl, sizeof(int));
                if (r < 0) {
                        bc_log_errorf("Failed to set TCP_KEEPINTVL: %s", strerror(errno));
                        return -errno;
                }
        }
        if (opts->tcp_keepcnt != 0) {
                r = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &opts->tcp_keepcnt, sizeof(int));
                if (r < 0) {
                        bc_log_errorf("Failed to set TCP_KEEPCNT: %s", strerror(errno));
                        return -errno;
                }
        }

        return 0;
}
