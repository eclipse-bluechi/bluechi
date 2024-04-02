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
#include "libbluechi/log/log.h"

bool test_log_target_to_str(LogFn fn, const char *expected_log_target) {
        const char *result = log_target_to_str(fn);
        if (result != NULL && expected_log_target != NULL && streq(result, expected_log_target)) {
                return true;
        }
        if (result == NULL && expected_log_target == NULL) {
                return true;
        }
        fprintf(stdout,
                "FAILED: string_to_log_level - Expected '%s', but got '%s'\n",
                expected_log_target,
                result);
        return false;
}

int main() {
        bool result = true;

        result = result && test_log_target_to_str(bc_log_to_journald_with_location, BC_LOG_TARGET_JOURNALD);
        result = result &&
                        test_log_target_to_str(bc_log_to_stderr_full_with_location, BC_LOG_TARGET_STDERR_FULL);
        result = result && test_log_target_to_str(bc_log_to_stderr_with_location, BC_LOG_TARGET_STDERR);
        result = result && test_log_target_to_str(NULL, NULL);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
