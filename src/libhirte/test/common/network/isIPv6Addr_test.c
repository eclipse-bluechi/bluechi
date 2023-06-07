/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/network.h"

#define bool_to_str(b) b ? "true" : "false"

bool test_isIPv6Addr(const char *in, bool expected) {
        bool result = isIPv6Addr(in);
        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: isIPv6Addr('%s') - Expected %s, but got %s\n",
                in,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}


int main() {
        bool result = true;

        result = result && test_isIPv6Addr(NULL, false);
        result = result && test_isIPv6Addr("", false);
        result = result && test_isIPv6Addr(".", false);
        result = result && test_isIPv6Addr("fe80::78b3:75ff:fe1b:6803", true);
        result = result && test_isIPv6Addr("1050:0000:0000:0000:0005:0600:300c:326b", true);
        result = result && test_isIPv6Addr("1050:0:0:0:5:600:300c:326b", true);
        result = result && test_isIPv6Addr("1050:0:0:0:5:600:300c:326b", true);
        result = result && test_isIPv6Addr("ff06:0:0:0:0:0:0:c3", true);
        result = result && test_isIPv6Addr("ff06::c3", true);
        result = result && test_isIPv6Addr("192.168.1.5", false);
        result = result && test_isIPv6Addr("10.10.10.5", false);
        result = result && test_isIPv6Addr("?10.10.10.3", false);
        result = result && test_isIPv6Addr("mydomain.com", false);
        result = result && test_isIPv6Addr("192.168.1.", false);
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
