/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/network.h"

#define bool_to_str(b) b ? "true" : "false"

bool test_isIPv4Addr(const char *in, bool expected) {
        bool result = isIPv4Addr(in);
        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: isIPv4Addr('%s') - Expected %s, but got %s\n",
                in,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}


int main() {
        bool result = true;

        result = result && test_isIPv4Addr(NULL, false);
        result = result && test_isIPv4Addr("", false);
        result = result && test_isIPv4Addr(".", false);
        result = result && test_isIPv4Addr("fe80::78b3:75ff:fe1b:6803", false);
        result = result && test_isIPv4Addr("192.168.1.5", true);
        result = result && test_isIPv4Addr("10.10.10.5", true);
        result = result && test_isIPv4Addr("?10.10.10.3", false);
        result = result && test_isIPv4Addr("mydomain.com", false);
        result = result && test_isIPv4Addr("192.168.1.", false);
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
