/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/string-util.h"

bool test_ends_with(const char *str, const char *suffix, bool does_end_with) {
        bool result = ends_with(str, suffix);
        if (result == does_end_with) {
                return true;
        }
        fprintf(stdout,
                "FAILED: ends_with('%s', '%s') - Expected %s, but got %s\n",
                str,
                suffix,
                bool_to_str(does_end_with),
                bool_to_str(result));
        return false;
}

int main() {
        bool result = true;

        result = result && test_ends_with(NULL, NULL, false);
        result = result && test_ends_with("", NULL, false);
        result = result && test_ends_with(NULL, "", false);
        result = result && test_ends_with("", "", true);
        result = result && test_ends_with("ctrl.conf", "", true);
        result = result && test_ends_with("ctrl.conf", ".conf", true);
        result = result && test_ends_with("ctrl.conf.backup", ".conf", false);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
