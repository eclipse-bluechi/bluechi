/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/network.h"
#include "libhirte/common/string-util.h"

bool test_get_address(const char *domain, char *ip, size_t ip_size, bool expected) {
        bool result = get_address(domain, ip, ip_size);
        // adjust the values for the below validation
        if (result == 0) {
                result = true;
        } else {
                result = false;
        }

        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: get_address('%s') - Expected %s, but got %s\n",
                domain,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}


int main() {
        bool result = true;
        char ip[INET_ADDRSTRLEN];

        result = result && test_get_address(NULL, ip, sizeof(ip), false);
        result = result && test_get_address(".", ip, sizeof(ip), false);
        result = result && test_get_address("redhat", ip, sizeof(ip), false);
        result = result && test_get_address("redhat.com", ip, sizeof(ip), true);
        result = result && test_get_address("8.8.8.8", ip, sizeof(ip), false);
        result = result && test_get_address("fe80::78b3:75ff:fe1b:6803", ip, sizeof(ip), false);
        result = result && test_get_address("?10.10.10.3", ip, sizeof(ip), false);
        result = result && test_get_address("192.168.1.", ip, sizeof(ip), false);
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
