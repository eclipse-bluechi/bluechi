/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/log/log.h"

bool test_string_to_log_level(const char *in, LogLevel expected_log_level) {
        LogLevel result = string_to_log_level(in);
        if (result == expected_log_level) {
                return true;
        }
        fprintf(stdout,
                "FAILED: string_to_log_level('%s') - Expected %s, but got %s\n",
                in,
                log_level_to_string(expected_log_level),
                log_level_to_string(result));
        return false;
}

int main() {
        bool result = true;

        result = result && test_string_to_log_level("DEBUG", LOG_LEVEL_DEBUG);
        result = result && test_string_to_log_level("INFO", LOG_LEVEL_INFO);
        result = result && test_string_to_log_level("ERROR", LOG_LEVEL_ERROR);
        result = result && test_string_to_log_level("WARN", LOG_LEVEL_WARN);

        result = result && test_string_to_log_level(NULL, LOG_LEVEL_INVALID);
        result = result && test_string_to_log_level("", LOG_LEVEL_INVALID);
        result = result && test_string_to_log_level("info", LOG_LEVEL_INVALID);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
