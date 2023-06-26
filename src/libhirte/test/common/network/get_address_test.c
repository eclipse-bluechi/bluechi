/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/network.h"
#include "libhirte/common/string-util.h"

/* test cases with invalid or unresolvable domains */
bool test_get_address_failure(const char *domain) {
        _cleanup_free_ char *ip = NULL;
        int result = get_address(domain, &ip);
        if (result >= 0) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - Expected a failure (result < 0), but got %d\n",
                        domain,
                        result);
                return false;
        }
        if (ip != NULL) {
                fprintf(stdout, "FAILED: get_address('%s') - Expected null, but got non-null (%s)\n", domain, ip);
                return false;
        }
        return true;
}

/* test cases where domain already contains an IPv4 or IPv6 */
bool test_get_address_success(const char *domain) {
        _cleanup_free_ char *ip = NULL;
        int result = get_address(domain, &ip);
        if (result != 0) {
                fprintf(stdout,
                        "FAILED: get_address('%s') - Expected a success (result == 0), but got %d (%s)\n",
                        domain,
                        result,
                        strerror(-result));
                return false;
        }
        if (ip != NULL) {
                fprintf(stdout, "FAILED: get_address('%s') - Expected null, but got non-null (%s)\n", domain, ip);
                return false;
        }
        return true;
}

/* test case for actually resolving a domain to an IP */
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
                fprintf(stdout, "FAILED: get_address('%s') - Expected non-null, but got null\n", domain);
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
        result = result && test_get_address_failure(NULL);
        result = result && test_get_address_failure(".");
        result = result && test_get_address_failure("redhat");
        result = result && test_get_address_failure("?10.10.10.3");
        result = result && test_get_address_failure("192.168.1.");
        result = result && test_get_address_success("8.8.8.8");
        result = result && test_get_address_success("fe80::78b3:75ff:fe1b:6803");
        result = result && test_get_address_localhost();
        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
