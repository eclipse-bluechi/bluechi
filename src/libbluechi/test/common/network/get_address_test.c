/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/network.h"
#include "libbluechi/common/string-util.h"

typedef struct get_address_test_params {
        char *hostname;
        bool use_ipv4;
        char *expected_resolved_addresses;
        int expected_result;
} get_address_test_params;


long unsigned int test_data_index = 0;
struct get_address_test_params test_data[] = {
        { NULL, true, NULL, -EINVAL },
        { ".", true, NULL, -1 },
        { "redhat", true, NULL, -1 },
        { "?10.10.10.3", true, NULL, -1 },
        { "192.168.1.", true, NULL, -1 },
        { "8.8.8.8", true, NULL, 0 },
        { "localhost.localdomain", true, "127.0.0.1", 0 },
};

int getaddrinfo_mock(
                UNUSED const char *name,
                UNUSED const char *service,
                UNUSED const struct addrinfo *req,
                struct addrinfo **pai) {

        void *offset;

        get_address_test_params current_test_case = test_data[test_data_index];
        if (current_test_case.expected_result < 0) {
                *pai = NULL;
                return current_test_case.expected_result;
        }

        /*
         * To emulate what getaddrinfo does, allocate both sockaddr_in and
         * addrinfo together so freeaddrinfo() will free the entire thing.
         */
        *pai = malloc0(sizeof(struct addrinfo) + sizeof(struct sockaddr_in6));
        offset = (void *) ((long) *pai + sizeof(struct addrinfo));

        if (current_test_case.use_ipv4) {
                struct sockaddr_in *addr = (struct sockaddr_in *) offset;
                (*pai)->ai_family = AF_INET;
                if (!inet_pton(AF_INET, current_test_case.expected_resolved_addresses, addr)) {
                        fprintf(stderr, "sockaddr_in AF_INET failed");
                        exit(1);
                }
                (*pai)->ai_addr = (struct sockaddr *) addr;
                (*pai)->ai_addrlen = sizeof(*addr);
        } else {
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *) offset;
                if (!addr) {
                        fprintf(stderr, "sockaddr_in6 AF_INET6 failed");
                        exit(1);
                }

                (*pai)->ai_family = AF_INET6;
                inet_pton(AF_INET6, current_test_case.expected_resolved_addresses, &addr);
                if (!inet_pton(AF_INET6, current_test_case.expected_resolved_addresses, &addr)) {
                        fprintf(stderr, "inet_pton AF_INET6 failed");
                        exit(1);
                }
                (*pai)->ai_addr = (struct sockaddr *) addr;
                (*pai)->ai_addrlen = sizeof(*addr);
        }

        return current_test_case.expected_result;
}

bool test_get_address(get_address_test_params params) {
        _cleanup_free_ char *ip = NULL;
        int result = get_address(params.hostname, &ip, getaddrinfo_mock);
        if ((result != params.expected_result) ||
            ((params.expected_resolved_addresses == NULL && ip != NULL) ||
             (params.expected_resolved_addresses != NULL && ip == NULL))) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - "
                        "Expected return value %d with resolved address '%s', but got return value %d with '%s'\n",
                        params.hostname,
                        params.expected_result,
                        params.expected_resolved_addresses,
                        result,
                        ip);
                return false;
        }
        return true;
}

int main() {
        bool result = true;
        for (test_data_index = 0; test_data_index < sizeof(test_data) / sizeof(get_address_test_params);
             test_data_index++) {
                result = result && test_get_address(test_data[test_data_index]);
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
