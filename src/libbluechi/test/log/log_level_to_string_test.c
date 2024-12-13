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

bool test_log_level_to_string(LogLevel in, const char *expected_log_level) {
        const char *lvl = log_level_to_string(in);
        if (lvl == NULL || !streq(expected_log_level, lvl)) {
                fprintf(stderr,
                        "FAILED: log_level_to_string('%d') - Expected %s, but got %s\n",
                        in,
                        expected_log_level,
                        lvl);
                return false;
        }
        return true;
}

int main() {
        bool result = true;

        result = result && test_log_level_to_string(LOG_LEVEL_DEBUG, "DEBUG");
        result = result && test_log_level_to_string(LOG_LEVEL_INFO, "INFO");
        result = result && test_log_level_to_string(LOG_LEVEL_ERROR, "ERROR");
        result = result && test_log_level_to_string(LOG_LEVEL_WARN, "WARN");
        result = result && test_log_level_to_string(LOG_LEVEL_INVALID, "INVALID");

        result = result && test_log_level_to_string(-1, "INVALID");
        result = result && test_log_level_to_string(99, "INVALID");

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
