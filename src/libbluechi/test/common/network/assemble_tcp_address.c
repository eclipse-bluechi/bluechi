/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/network.h"


#ifndef __FUNCTION_NAME__
#        define __FUNCTION_NAME__ __func__
#endif

int build_sockaddr_in6(const char *host_in, uint16_t port_in, struct sockaddr_in6 *ret) {
        int r = 0;

        memset(ret, 0, sizeof(*ret));
        ret->sin6_family = AF_INET6;
        ret->sin6_port = htons(port_in);

        r = inet_pton(AF_INET6, host_in, &ret->sin6_addr);
        if (r < 1) {
                fprintf(stdout, "Invalid host option '%s'\n", host_in);
                return r;
        }

        return 0;
}

int build_sockaddr_in(const char *host_in, uint16_t port_in, struct sockaddr_in *ret) {
        int r = 0;

        memset(ret, 0, sizeof(*ret));
        ret->sin_family = AF_INET;
        ret->sin_port = htons(port_in);

        r = inet_pton(AF_INET, host_in, &ret->sin_addr);
        if (r < 1) {
                fprintf(stdout, "Invalid host option '%s'\n", host_in);
                return r;
        }

        return 0;
}

bool test_assemble_tcp_address_v6(const struct sockaddr_in6 *input, const char *expectedResult) {
        _cleanup_free_ char *result = assemble_tcp_address_v6(input);
        if ((result == NULL && expectedResult != NULL) || (result != NULL && !streq(result, expectedResult))) {
                _cleanup_free_ char *msg = NULL;
                int r = asprintf(
                                &msg,
                                "FAILED: %s\n\tExpected: %s\n\tGot: %s\n",
                                __FUNCTION_NAME__,
                                expectedResult,
                                result);
                if (r < 0) {
                        fprintf(stderr, "OOM: %s failed", __FUNCTION_NAME__);
                        return false;
                }
                if (msg != NULL) {
                        fprintf(stderr, "%s", msg);
                        return false;
                }
        }
        return true;
}

bool test_assemble_tcp_address(const struct sockaddr_in *input, const char *expectedResult) {
        _cleanup_free_ char *result = assemble_tcp_address(input);
        if ((result == NULL && expectedResult != NULL) || (result != NULL && !streq(result, expectedResult))) {
                _cleanup_free_ char *msg = NULL;
                int r = asprintf(
                                &msg,
                                "FAILED: %s\n\tExpected: %s\n\tGot: %s\n",
                                __FUNCTION_NAME__,
                                expectedResult,
                                result);
                if (r < 0) {
                        fprintf(stderr, "OOM: %s failed", __FUNCTION_NAME__);
                        return false;
                }
                if (msg != NULL) {
                        fprintf(stderr, "%s", msg);
                        return false;
                }
        }
        return true;
}

int main() {
        bool result = true;
        struct sockaddr_in host;
        struct sockaddr_in6 host6;

        result = result && test_assemble_tcp_address(NULL, NULL);

        build_sockaddr_in("127.0.0.1", 808, &host);
        result = result && test_assemble_tcp_address(&host, "tcp:host=127.0.0.1,port=808");

        build_sockaddr_in6("::1", 808, &host6);
        result = result && test_assemble_tcp_address_v6(&host6, "tcp:host=::1,port=808");

        build_sockaddr_in("192.168.178.1", 808, &host);
        result = result && test_assemble_tcp_address(&host, "tcp:host=192.168.178.1,port=808");

        build_sockaddr_in("", 808, &host);
        result = result && test_assemble_tcp_address(&host, "tcp:host=0.0.0.0,port=808");

        build_sockaddr_in("I am invalid", 808, &host);
        result = result && test_assemble_tcp_address(&host, "tcp:host=0.0.0.0,port=808");

        build_sockaddr_in("I am invalid", -1, &host);
        result = result && test_assemble_tcp_address(&host, "tcp:host=0.0.0.0,port=65535");

        if (!result) {
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}
