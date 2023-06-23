/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/network.h"
#include "libhirte/common/string-util.h"

bool test_get_address_failure(const char *domain, int expected_ret) {
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
        if (ip != NULL) {
                fprintf(stdout, "FAILED: get_address('%s') - Expected null, but got non-null (%s)", domain, ip);
                return false;
        }
        return true;
}

bool test_get_address_localhost() {
        const char *domain = "localhost";
        int expected_ret = 0;
        const char *expected_addressv4 = "127.0.0.1";
        const char *expected_addressv6 = "::1";

        _cleanup_free_ char *ip = NULL;
        int result = get_address("localhost", &ip);
        if (result != expected_ret) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - Expected return code %d, but got %d (%s)\n",
                        domain,
                        expected_ret,
                        result,
                        strerror(-result));
                return false;
        }
        if (ip == NULL) {
                fprintf(stdout, "FAILED: get_address('%s') - Expected non-null, but got null", domain);
                return false;
        }
        if (!streq(ip, expected_addressv4) && !streq(ip, expected_addressv6)) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - Expected either %s or %s, but got %s\n",
                        domain,
                        expected_addressv4,
                        expected_addressv6,
                        ip);
                return false;
        }
        return true;
}


int main() {
        bool result = true;
        result = result && test_get_address_failure(NULL, -EINVAL);
        result = result && test_get_address_failure(".", -ENOENT);
        result = result && test_get_address_failure("redhat", -ENOENT);
        result = result && test_get_address_failure("8.8.8.8", 0);
        result = result && test_get_address_failure("fe80::78b3:75ff:fe1b:6803", 0);
        result = result && test_get_address_failure("?10.10.10.3", -ENOENT);
        result = result && test_get_address_failure("192.168.1.", -ENOENT);
        result = result && test_get_address_localhost();
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
