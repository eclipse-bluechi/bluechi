/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/network.h"
#include "libbluechi/common/string-util.h"

bool test_is_ipv4(const char *in, bool expected) {
        bool result = is_ipv4(in);
        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: is_ipv4('%s') - Expected %s, but got %s\n",
                in,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}


int main() {
        bool result = true;

        result = result && test_is_ipv4(NULL, false);
        result = result && test_is_ipv4("", false);
        result = result && test_is_ipv4(".", false);
        result = result && test_is_ipv4("fe80::78b3:75ff:fe1b:6803", false);
        result = result && test_is_ipv4("192.168.1.5", true);
        result = result && test_is_ipv4("10.10.10.5", true);
        result = result && test_is_ipv4("?10.10.10.3", false);
        result = result && test_is_ipv4("mydomain.com", false);
        result = result && test_is_ipv4("192.168.1.", false);
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
