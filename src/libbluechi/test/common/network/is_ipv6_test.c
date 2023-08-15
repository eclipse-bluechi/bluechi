/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/network.h"
#include "libbluechi/common/string-util.h"

bool test_is_ipv6(const char *in, bool expected) {
        bool result = is_ipv6(in);
        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: is_ipv6('%s') - Expected %s, but got %s\n",
                in,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}


int main() {
        bool result = true;

        result = result && test_is_ipv6(NULL, false);
        result = result && test_is_ipv6("", false);
        result = result && test_is_ipv6(".", false);
        result = result && test_is_ipv6("fe80::78b3:75ff:fe1b:6803", true);
        result = result && test_is_ipv6("1050:0000:0000:0000:0005:0600:300c:326b", true);
        result = result && test_is_ipv6("1050:0:0:0:5:600:300c:326b", true);
        result = result && test_is_ipv6("1050:0:0:0:5:600:300c:326b", true);
        result = result && test_is_ipv6("ff06:0:0:0:0:0:0:c3", true);
        result = result && test_is_ipv6("ff06::c3", true);
        result = result && test_is_ipv6("192.168.1.5", false);
        result = result && test_is_ipv6("10.10.10.5", false);
        result = result && test_is_ipv6("?10.10.10.3", false);
        result = result && test_is_ipv6("mydomain.com", false);
        result = result && test_is_ipv6("192.168.1.", false);
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
