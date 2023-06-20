/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/network.h"
#include "libhirte/common/string-util.h"

bool test_get_address(const char *domain, const char *expected_address, int expected_ret) {
        _cleanup_free_ char *ip = NULL;
        int result = get_address(domain, &ip);
        if (result != expected_ret) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - Expected return code %d, but got %d (%s)\n",
                        domain,
                        expected_ret,
                        result,
                        strerror(-result));
                return false;
        }
        if (expected_address == NULL && ip != NULL) {
                fprintf(stdout, "FAILED: get_address('%s') - Expected null, but got non-null (%s)", domain, ip);
                return false;
        }
        if (expected_address != NULL && ip == NULL) {
                fprintf(stdout, "FAILED: get_address('%s') - Expected non-null, but got null", domain);
                return false;
        }
        if (expected_address != NULL && ip != NULL && !streq(ip, expected_address)) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - Expected %s, but got %s\n",
                        domain,
                        expected_address,
                        ip);
                return false;
        }
        return true;
}


int main() {
        bool result = true;
        result = result && test_get_address(NULL, NULL, -EINVAL);
        result = result && test_get_address(".", NULL, -ENOENT);
        result = result && test_get_address("redhat", NULL, -ENOENT);
        result = result && test_get_address("localhost", "127.0.0.1", 0);
        result = result && test_get_address("8.8.8.8", NULL, 0);
        result = result && test_get_address("fe80::78b3:75ff:fe1b:6803", NULL, 0);
        result = result && test_get_address("?10.10.10.3", NULL, -ENOENT);
        result = result && test_get_address("192.168.1.", NULL, -ENOENT);
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
