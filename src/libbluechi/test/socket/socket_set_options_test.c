/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "libbluechi/common/common.h"
#include "libbluechi/socket.h"


static int create_test_tcp_socket() {
        int fd = socket(AF_INET6, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
        if (fd < 0) {
                int errsv = errno;
                return -errsv;
        }
        return fd;
}

bool test_socket_set_options_no_tcp_socket() {
        _cleanup_fd_ int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
                fprintf(stderr, "Unexpected error while opening socket: %s\n", strerror(-errno));
                return false;
        }

        _cleanup_free_ SocketOptions *opts = socket_options_new();
        int r = socket_set_options(fd, opts);
        if (r != 0) {
                fprintf(stderr, "%s: Expected return code 0 for UNIX socket, but got %d\n", __func__, r);
                return false;
        }

        return true;
}

bool test_socket_set_options_null() {
        _cleanup_fd_ int fd = create_test_tcp_socket();
        if (fd < 0) {
                fprintf(stderr, "Unexpected error while opening socket: %s", strerror(-errno));
                return false;
        }

        int r = socket_set_options(fd, NULL);
        if (r != -EINVAL) {
                fprintf(stderr, "%s: Expected return code %d for UNIX socket, but got %d", __func__, -EINVAL, r);
                return false;
        }

        return true;
}

bool test_socket_set_options_ip_recv_err(bool ip_recv_err, int expected_val) {
        _cleanup_fd_ int fd = create_test_tcp_socket();
        if (fd < 0) {
                fprintf(stderr, "Unexpected error while opening socket: %s\n", strerror(-errno));
                return false;
        }

        _cleanup_free_ SocketOptions *opts = socket_options_new();
        socket_options_set_ip_recverr(opts, ip_recv_err);
        int r = socket_set_options(fd, opts);
        if (r != 0) {
                fprintf(stderr, "%s: Expected return code 0, but got %d\n", __func__, r);
                return false;
        }

        int val = 0;
        socklen_t len = sizeof(int);
        getsockopt(fd, IPPROTO_IP, IP_RECVERR, &val, &len);
        if (val != expected_val) {
                fprintf(stderr, "%s: Expected IP_RECVERR to be %d, but got %d\n", __func__, expected_val, val);
                return false;
        }

        return true;
}

bool test_socket_set_options_keepalive(
                const char *keepidle,
                const char *keepintvl,
                const char *keepcnt,
                int expected_keepalive,
                int expected_keepidle,
                int expected_keepintvl,
                int expected_keepcnt) {
        _cleanup_fd_ int fd = create_test_tcp_socket();
        if (fd < 0) {
                fprintf(stderr, "Unexpected error while opening socket: %s\n", strerror(-errno));
                return false;
        }

        _cleanup_free_ SocketOptions *opts = socket_options_new();
        socket_options_set_tcp_keepidle(opts, keepidle);
        socket_options_set_tcp_keepintvl(opts, keepintvl);
        socket_options_set_tcp_keepcnt(opts, keepcnt);

        int r = socket_set_options(fd, opts);
        if (r != 0) {
                fprintf(stderr, "%s: Expected return code 0, but got %d\n", __func__, r);
                return false;
        }

        bool result = true;
        int val = 0;
        socklen_t len = sizeof(int);
        getsockopt(fd, SOL_TCP, TCP_NODELAY, &val, &len);
        if (val != 1) {
                fprintf(stderr, "%s: Expected TCP_NODELAY to be 1, but got %d\n", __func__, val);
                result = false;
        }
        val = 0;
        getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, &len);
        if (val != expected_keepalive) {
                fprintf(stderr,
                        "%s: Expected SO_KEEPALIVE to be %d, but got %d\n",
                        __func__,
                        expected_keepalive,
                        val);
                result = false;
        }
        val = 0;
        getsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, &len);
        /* system default is used when expected_keepidle == 0 */
        if (val != expected_keepidle && expected_keepidle != 0) {
                fprintf(stderr,
                        "%s: Expected TCP_KEEPIDLE to be %d, but got %d\n",
                        __func__,
                        expected_keepidle,
                        val);
                result = false;
        }
        val = 0;
        /* system default is used when expected_keepintvl == 0 */
        getsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, &len);
        if (val != expected_keepintvl && expected_keepintvl != 0) {
                fprintf(stderr,
                        "%s: Expected TCP_KEEPINTVL to be %d, but got %d\n",
                        __func__,
                        expected_keepintvl,
                        val);
                result = false;
        }
        val = 0;
        getsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, &len);
        /* system default is used when expected_keepcnt == 0 */
        if (val != expected_keepcnt && expected_keepcnt != 0) {
                fprintf(stderr,
                        "%s: Expected TCP_KEEPCNT to be %d, but got %d\n",
                        __func__,
                        expected_keepcnt,
                        val);
                result = false;
        }

        return result;
}


int main() {
        bool result = true;

        result = result && test_socket_set_options_no_tcp_socket();
        result = result && test_socket_set_options_null();

        result = result && test_socket_set_options_ip_recv_err(false, 0);
        result = result && test_socket_set_options_ip_recv_err(true, 1);

        result = result && test_socket_set_options_keepalive("0", "0", "0", 1, 0, 0, 0);
        result = result && test_socket_set_options_keepalive("1", "0", "0", 1, 1, 0, 0);
        result = result && test_socket_set_options_keepalive("0", "1", "0", 1, 0, 1, 0);
        result = result && test_socket_set_options_keepalive("0", "0", "6", 1, 0, 0, 6);
        result = result && test_socket_set_options_keepalive("10", "5", "8", 1, 10, 5, 8);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
