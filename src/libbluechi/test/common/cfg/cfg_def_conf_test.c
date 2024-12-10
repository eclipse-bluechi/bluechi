/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/protocol.h"
#include "libbluechi/common/string-util.h"
#include "libbluechi/log/log.h"

bool test_cfg_agent_def_conf() {
        struct config *config = NULL;
        cfg_initialize(&config);

        int r = cfg_agent_def_conf(config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error while setting agent default values: %s", strerror(-r));
                cfg_dispose(config);
                return false;
        }

        bool result = true;
        const char *value = NULL;

        /* Common options */
        value = cfg_get_value(config, CFG_LOG_LEVEL);
        if (!streq(value, log_level_to_string(LOG_LEVEL_INFO))) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_LOG_LEVEL,
                        log_level_to_string(LOG_LEVEL_INFO),
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_LOG_TARGET);
        if (!streq(value, BC_LOG_TARGET_JOURNALD)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_LOG_TARGET,
                        BC_LOG_TARGET_JOURNALD,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_LOG_IS_QUIET);
        if (value != NULL) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_LOG_IS_QUIET,
                        "NULL",
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_IP_RECEIVE_ERRORS);
        if (!streq(value, BC_DEFAULT_IP_RECEIVE_ERROR)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_IP_RECEIVE_ERRORS,
                        BC_DEFAULT_IP_RECEIVE_ERROR,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_TCP_KEEPALIVE_TIME);
        if (!streq(value, BC_DEFAULT_TCP_KEEPALIVE_TIME)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_TCP_KEEPALIVE_TIME,
                        BC_DEFAULT_TCP_KEEPALIVE_TIME,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_TCP_KEEPALIVE_INTERVAL);
        if (!streq(value, BC_DEFAULT_TCP_KEEPALIVE_INTERVAL)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_TCP_KEEPALIVE_INTERVAL,
                        BC_DEFAULT_TCP_KEEPALIVE_INTERVAL,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_TCP_KEEPALIVE_COUNT);
        if (!streq(value, BC_DEFAULT_TCP_KEEPALIVE_COUNT)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_TCP_KEEPALIVE_COUNT,
                        BC_DEFAULT_TCP_KEEPALIVE_COUNT,
                        value);
                result = false;
        }

        /* Agent specific options */
        value = cfg_get_value(config, CFG_NODE_NAME);
        if (value == NULL) {
                fprintf(stderr,
                        "Expected config option %s to have set a default value, but got 'NULL'\n",
                        CFG_NODE_NAME);
                result = false;
        }
        value = cfg_get_value(config, CFG_CONTROLLER_HOST);
        if (!streq(value, BC_DEFAULT_HOST)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_CONTROLLER_HOST,
                        BC_DEFAULT_HOST,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_CONTROLLER_PORT);
        if (!streq(value, BC_DEFAULT_PORT)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_CONTROLLER_PORT,
                        BC_DEFAULT_PORT,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_HEARTBEAT_INTERVAL);
        if (!streq(value, AGENT_DEFAULT_HEARTBEAT_INTERVAL_MSEC)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_HEARTBEAT_INTERVAL,
                        AGENT_DEFAULT_HEARTBEAT_INTERVAL_MSEC,
                        value);
                result = false;
        }

        cfg_dispose(config);
        return result;
}

bool test_cfg_controller_def_conf() {
        struct config *config = NULL;
        cfg_initialize(&config);

        int r = cfg_controller_def_conf(config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error while setting agent default values: %s", strerror(-r));
                cfg_dispose(config);
                return false;
        }

        bool result = true;
        const char *value = NULL;

        /* Common options */
        value = cfg_get_value(config, CFG_LOG_LEVEL);
        if (!streq(value, log_level_to_string(LOG_LEVEL_INFO))) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_LOG_LEVEL,
                        log_level_to_string(LOG_LEVEL_INFO),
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_LOG_TARGET);
        if (!streq(value, BC_LOG_TARGET_JOURNALD)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_LOG_TARGET,
                        BC_LOG_TARGET_JOURNALD,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_LOG_IS_QUIET);
        if (value != NULL) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_LOG_IS_QUIET,
                        "NULL",
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_IP_RECEIVE_ERRORS);
        if (!streq(value, BC_DEFAULT_IP_RECEIVE_ERROR)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_IP_RECEIVE_ERRORS,
                        BC_DEFAULT_IP_RECEIVE_ERROR,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_TCP_KEEPALIVE_TIME);
        if (!streq(value, BC_DEFAULT_TCP_KEEPALIVE_TIME)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_TCP_KEEPALIVE_TIME,
                        BC_DEFAULT_TCP_KEEPALIVE_TIME,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_TCP_KEEPALIVE_INTERVAL);
        if (!streq(value, BC_DEFAULT_TCP_KEEPALIVE_INTERVAL)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_TCP_KEEPALIVE_INTERVAL,
                        BC_DEFAULT_TCP_KEEPALIVE_INTERVAL,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_TCP_KEEPALIVE_COUNT);
        if (!streq(value, BC_DEFAULT_TCP_KEEPALIVE_COUNT)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_TCP_KEEPALIVE_COUNT,
                        BC_DEFAULT_TCP_KEEPALIVE_COUNT,
                        value);
                result = false;
        }

        /* Controller specific options */
        value = cfg_get_value(config, CFG_CONTROLLER_USE_TCP);
        if (!streq(value, CONTROLLER_DEFAULT_USE_TCP)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_CONTROLLER_USE_TCP,
                        CONTROLLER_DEFAULT_USE_TCP,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_CONTROLLER_USE_UDS);
        if (!streq(value, CONTROLLER_DEFAULT_USE_UDS)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_CONTROLLER_USE_UDS,
                        CONTROLLER_DEFAULT_USE_UDS,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_CONTROLLER_PORT);
        if (!streq(value, BC_DEFAULT_PORT)) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_CONTROLLER_PORT,
                        BC_DEFAULT_PORT,
                        value);
                result = false;
        }
        value = cfg_get_value(config, CFG_ALLOWED_NODE_NAMES);
        if (value != NULL) {
                fprintf(stderr,
                        "Expected config option %s to have default value '%s', but got '%s'\n",
                        CFG_ALLOWED_NODE_NAMES,
                        "NULL",
                        value);
                result = false;
        }

        cfg_dispose(config);
        return result;
}

int main() {
        bool result = true;

        result = result && test_cfg_agent_def_conf();
        result = result && test_cfg_controller_def_conf();

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
