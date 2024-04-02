/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/bus/utils.h"
#include "libbluechi/common/string-util.h"

bool test_bus_id_is_valid(const char *name, bool expected_is_valid) {
        bool got_is_valid = bus_id_is_valid(name);
        if (got_is_valid != expected_is_valid) {
                fprintf(stderr,
                        "FAILED: expected bus name '%s' to be %s, but got %s\n",
                        name,
                        bool_to_str(expected_is_valid),
                        bool_to_str(got_is_valid));
                return false;
        }
        return true;
}

int main() {
        bool result = true;

        result = result && test_bus_id_is_valid(NULL, false);
        result = result && test_bus_id_is_valid("", false);
        result = result && test_bus_id_is_valid(":", false);
        result = result && test_bus_id_is_valid(":.", false);
        result = result && test_bus_id_is_valid(":1.", false);
        result = result && test_bus_id_is_valid(":1.3.", false);
        result = result && test_bus_id_is_valid("1", false);
        result = result && test_bus_id_is_valid("1.", false);
        result = result && test_bus_id_is_valid("1.3", false);
        result = result && test_bus_id_is_valid(":1..3", false);
        result = result && test_bus_id_is_valid("org.eclipse.bluechi", false);
        result = result &&
                        test_bus_id_is_valid(
                                        ":1.1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111",
                                        false);

        result = result && test_bus_id_is_valid(":1.1", true);
        result = result && test_bus_id_is_valid(":1.12345", true);
        result = result && test_bus_id_is_valid(":1.123.45", true);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
