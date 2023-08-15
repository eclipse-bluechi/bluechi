/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/log/log.h"


typedef struct log_test_param {
        char *log_func;
        char *log_level;
        char *is_quiet;

        LogFn expected_log_func;
        LogLevel expected_log_level;
        bool expected_is_quiet;
} log_test_param;


struct log_test_param test_data[] = {
        // test valid log targets
        { "journald", "INFO", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "stderr", "INFO", "false", bc_log_to_stderr_with_location, LOG_LEVEL_INFO, false },
        { "stderr-full", "INFO", "false", bc_log_to_stderr_full_with_location, LOG_LEVEL_INFO, false },
        // test valid log levels
        { "journald", "DEBUG", "false", bc_log_to_journald_with_location, LOG_LEVEL_DEBUG, false },
        { "journald", "INFO", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "journald", "ERROR", "false", bc_log_to_journald_with_location, LOG_LEVEL_ERROR, false },
        { "journald", "WARN", "false", bc_log_to_journald_with_location, LOG_LEVEL_WARN, false },
        // test valid is quiet
        { "journald", "INFO", "true", bc_log_to_journald_with_location, LOG_LEVEL_INFO, true },

        // test invalid log targets: should result in a default log function
        { NULL, "INFO", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "", "INFO", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "stderr-ful", "INFO", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },

        // test invalid log levels: should result in a default log function
        { "journald", NULL, "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "journald", "", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "journald", "info", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "journald", "just wrong", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },

        // test is quiet: different string values interpreted as true
        { "journald", "INFO", "true", bc_log_to_journald_with_location, LOG_LEVEL_INFO, true },
        { "journald", "INFO", "t", bc_log_to_journald_with_location, LOG_LEVEL_INFO, true },
        { "journald", "INFO", "yes", bc_log_to_journald_with_location, LOG_LEVEL_INFO, true },
        { "journald", "INFO", "y", bc_log_to_journald_with_location, LOG_LEVEL_INFO, true },
        { "journald", "INFO", "on", bc_log_to_journald_with_location, LOG_LEVEL_INFO, true },
        // test is quiet: invalid values should result in false
        { "journald", "INFO", "f", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "journald", "INFO", "false", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
        { "journald", "INFO", "invalid", bc_log_to_journald_with_location, LOG_LEVEL_INFO, false },
};


bool test_bc_log_init(log_test_param test_case) {
        struct config *config = NULL;
        cfg_initialize(&config);
        cfg_set_value(config, CFG_LOG_LEVEL, test_case.log_level);
        cfg_set_value(config, CFG_LOG_TARGET, test_case.log_func);
        cfg_set_value(config, CFG_LOG_IS_QUIET, test_case.is_quiet);

        bc_log_init(config);

        bool result = true;
        if (bc_log_get_level() != test_case.expected_log_level) {
                fprintf(stdout, "FAILED: Wrong log level initialized for value '%s'\n", test_case.log_level);
                result = false;
        }
        if (bc_log_get_log_fn() != test_case.expected_log_func) {
                fprintf(stdout, "FAILED: Wrong log function initialized for value '%s'\n", test_case.log_func);
                result = false;
        }
        if (bc_log_get_quiet() != test_case.expected_is_quiet) {
                fprintf(stdout, "FAILED: Wrongly initialized log is quiet for value '%s'\n", test_case.is_quiet);
                result = false;
        }

        cfg_dispose(config);
        return result;
}

int main() {
        bool result = true;

        for (unsigned int i = 0; i < sizeof(test_data) / sizeof(log_test_param); i++) {
                result = result && test_bc_log_init(test_data[i]);
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
