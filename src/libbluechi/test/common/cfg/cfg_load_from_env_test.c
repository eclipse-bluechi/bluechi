/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"

void dispose_env() {
        unsetenv("BC_LOG_LEVEL");
        unsetenv("BC_LOG_TARGET");
        unsetenv("BC_LOG_IS_QUIET");
}

void test_parse_config_from_env() {
        const char *LOG_LEVEL_DEBUG = "DEBUG";
        const char *LOG_TARGET_STDERR = "stderr";
        const char *LOG_IS_QUIET_OFF = "false";

        struct config *config = NULL;
        cfg_initialize(&config);

        // initialize environment
        setenv("BC_LOG_LEVEL", LOG_LEVEL_DEBUG, 1);
        setenv("BC_LOG_TARGET", LOG_TARGET_STDERR, 1);
        setenv("BC_LOG_IS_QUIET", LOG_IS_QUIET_OFF, 1);

        // parse config from environment
        cfg_load_from_env(config);

        const char *value = NULL;

        value = cfg_get_value(config, CFG_LOG_LEVEL);
        assert(value != NULL);
        assert(streq(value, LOG_LEVEL_DEBUG));

        value = cfg_get_value(config, CFG_LOG_TARGET);
        assert(value != NULL);
        assert(streq(value, LOG_TARGET_STDERR));

        value = cfg_get_value(config, CFG_LOG_IS_QUIET);
        assert(value != NULL);
        assert(streq(value, LOG_IS_QUIET_OFF));

        cfg_dispose(config);
        dispose_env();
}

void test_env_override_config() {
        const char *LOG_LEVEL_DEBUG = "DEBUG";
        const char *LOG_LEVEL_INFO = "INFO";
        const char *LOG_TARGET_STDERR = "stderr";
        const char *LOG_IS_QUIET_ON = "on";

        struct config *config = NULL;
        cfg_initialize(&config);

        // initialize default configuration
        cfg_set_value(config, CFG_LOG_LEVEL, LOG_LEVEL_DEBUG);
        cfg_set_value(config, CFG_LOG_TARGET, LOG_TARGET_STDERR);
        cfg_set_value(config, CFG_LOG_IS_QUIET, LOG_IS_QUIET_ON);

        // initialize environment
        // log level should be overridden
        setenv("BC_LOG_LEVEL", LOG_LEVEL_INFO, 1);
        // empty string in shell is considered an empty value, so log target should not be overridden
        setenv("BC_LOG_TARGET", "", 1);
        // log quiet is not defined in env, so it should not be overridden

        // parse config from environment
        cfg_load_from_env(config);

        const char *value = NULL;

        value = cfg_get_value(config, CFG_LOG_LEVEL);
        assert(value != NULL);
        assert(streq(value, LOG_LEVEL_INFO));

        value = cfg_get_value(config, CFG_LOG_TARGET);
        assert(value != NULL);
        assert(streq(value, LOG_TARGET_STDERR));

        value = cfg_get_value(config, CFG_LOG_IS_QUIET);
        assert(value != NULL);
        assert(streq(value, LOG_IS_QUIET_ON));

        cfg_dispose(config);
        dispose_env();
}


int main() {
        test_parse_config_from_env();
        test_env_override_config();

        return EXIT_SUCCESS;
}
