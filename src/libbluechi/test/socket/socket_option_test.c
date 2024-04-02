/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libbluechi/common/common.h"
#include "libbluechi/socket.h"


bool test_socket_options_set_tcp_keepidle(SocketOptions *opts, const char *input, int expected_ret) {
        int r = socket_options_set_tcp_keepidle(opts, input);
        if (r != expected_ret) {
                fprintf(stderr,
                        "%s: Expected return value %d for input '%s', but got %d",
                        __func__,
                        expected_ret,
                        input,
                        r);
                return false;
        }
        return true;
}

bool test_socket_options_set_tcp_keepintvl(SocketOptions *opts, const char *input, int expected_ret) {
        int r = socket_options_set_tcp_keepintvl(opts, input);
        if (r != expected_ret) {
                fprintf(stderr,
                        "%s: Expected return value %d for input '%s', but got %d",
                        __func__,
                        expected_ret,
                        input,
                        r);
                return false;
        }
        return true;
}

bool test_socket_options_set_tcp_keepcnt(SocketOptions *opts, const char *input, int expected_ret) {
        int r = socket_options_set_tcp_keepcnt(opts, input);
        if (r != expected_ret) {
                fprintf(stderr,
                        "%s: Expected return value %d for input '%s', but got %d",
                        __func__,
                        expected_ret,
                        input,
                        r);
                return false;
        }
        return true;
}

bool test_socket_options_set_ip_recverr(SocketOptions *opts, bool input, int expected_ret) {
        int r = socket_options_set_ip_recverr(opts, input);
        if (r != expected_ret) {
                fprintf(stderr,
                        "%s: Expected return value %d for input '%d', but got %d",
                        __func__,
                        expected_ret,
                        input,
                        r);
                return false;
        }
        return true;
}

int main() {
        bool result = true;

        _cleanup_free_ SocketOptions *opts = socket_options_new();

        result = result && test_socket_options_set_tcp_keepidle(NULL, "", -EINVAL);
        result = result && test_socket_options_set_tcp_keepidle(opts, "abc123", -EINVAL);
        result = result && test_socket_options_set_tcp_keepidle(opts, "-1", -EINVAL);
        result = result && test_socket_options_set_tcp_keepidle(opts, "0", 0);
        result = result && test_socket_options_set_tcp_keepidle(opts, "10", 0);
        result = result && test_socket_options_set_tcp_keepidle(opts, "32767", 0);
        result = result && test_socket_options_set_tcp_keepidle(opts, "32768", -EINVAL);

        result = result && test_socket_options_set_tcp_keepintvl(NULL, "", -EINVAL);
        result = result && test_socket_options_set_tcp_keepintvl(opts, "abc123", -EINVAL);
        result = result && test_socket_options_set_tcp_keepintvl(opts, "-1", -EINVAL);
        result = result && test_socket_options_set_tcp_keepintvl(opts, "0", 0);
        result = result && test_socket_options_set_tcp_keepintvl(opts, "10", 0);
        result = result && test_socket_options_set_tcp_keepintvl(opts, "32767", 0);
        result = result && test_socket_options_set_tcp_keepintvl(opts, "32768", -EINVAL);

        result = result && test_socket_options_set_tcp_keepcnt(NULL, "", -EINVAL);
        result = result && test_socket_options_set_tcp_keepcnt(opts, "abc123", -EINVAL);
        result = result && test_socket_options_set_tcp_keepcnt(opts, "-1", -EINVAL);
        result = result && test_socket_options_set_tcp_keepcnt(opts, "0", 0);
        result = result && test_socket_options_set_tcp_keepcnt(opts, "10", 0);
        result = result && test_socket_options_set_tcp_keepcnt(opts, "127", 0);
        result = result && test_socket_options_set_tcp_keepcnt(opts, "128", -EINVAL);

        result = result && test_socket_options_set_ip_recverr(NULL, true, -EINVAL);
        result = result && test_socket_options_set_ip_recverr(opts, true, 0);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
