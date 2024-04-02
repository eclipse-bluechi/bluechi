/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "agent/agent.h"


void print_error_result(const char *func_name, bool expected, bool actual) {
        fprintf(stderr,
                "FAILED: expected %s to return '%s', but got '%s'\n",
                func_name,
                bool_to_str(expected),
                bool_to_str(actual));
}

void print_agent_field_error(
                const char *func_name, const char *field_name, const char *expected, const char *actual) {
        fprintf(stderr,
                "FAILED: expected %s to set 'agent->%s' to '%s', but got '%s'\n",
                func_name,
                field_name,
                expected,
                actual);
}

bool check_str(const char *func_name, const char *field_name, const char *expected, const char *actual) {
        if (expected == NULL && actual == NULL) {
                return true;
        }
        if ((expected == NULL && actual != NULL) || (expected != NULL && actual == NULL) ||
            !streq(expected, actual)) {
                print_agent_field_error(func_name, field_name, expected, actual);
                return false;
        }
        return true;
}

bool check_agent(
                const char *func_name,
                Agent *agent,
                const char *expected_name,
                const char *expected_host,
                uint16_t expected_port,
                const char *expected_address,
                long int expected_heartbeat_interval) {
        bool result = true;
        result = result && check_str(func_name, "name", expected_name, agent->name);
        result = result && check_str(func_name, "host", expected_host, agent->host);
        result = result &&
                        check_str(func_name, "controller_address", expected_address, agent->controller_address);
        if (expected_port != agent->port) {
                _cleanup_free_ char *actual = NULL;
                int r = asprintf(&actual, "%d", agent->port);
                if (r < 0) {
                        fprintf(stderr,
                                "FAILED: Unexpected error when parsing actual port to string: %s",
                                strerror(-r));
                        return false;
                }

                _cleanup_free_ char *expected = NULL;
                r = asprintf(&expected, "%d", expected_port);
                if (r < 0) {
                        fprintf(stderr,
                                "FAILED: Unexpected error when parsing expected port to string: %s",
                                strerror(-r));
                        return false;
                }
                print_agent_field_error(func_name, "port", expected, actual);
                result = false;
        }
        if (expected_heartbeat_interval != agent->heartbeat_interval_msec) {
                _cleanup_free_ char *intvl_actual = NULL;
                int r = asprintf(&intvl_actual, "%ld", agent->heartbeat_interval_msec);
                if (r < 0) {
                        fprintf(stderr,
                                "FAILED: Unexpected error when parsing actual heartbeat intvl to string: %s",
                                strerror(-r));
                        return false;
                }

                _cleanup_free_ char *intvl_expected = NULL;
                r = asprintf(&intvl_expected, "%ld", expected_heartbeat_interval);
                if (r < 0) {
                        fprintf(stderr,
                                "FAILED: Unexpected error when parsing expected heartbeat intvl to string: %s",
                                strerror(-r));
                        return false;
                }
                print_agent_field_error(func_name, "heartbeat_interval_msec", intvl_expected, intvl_actual);
                result = false;
        }

        // socket options have private members and can't be evaluated at the moment

        return result;
}

bool test_agent_apply_config_none() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        bool result = agent_apply_config(agent);
        if (!result) {
                print_error_result(__func__, true, result);
                return false;
        }

        return check_agent(__func__, agent, NULL, NULL, 0, NULL, 0);
}

bool test_agent_apply_config_valid_all() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(agent->config, CFG_NODE_NAME, "node-foo");
        cfg_set_value(agent->config, CFG_CONTROLLER_HOST, "127.0.0.1");
        cfg_set_value(agent->config, CFG_CONTROLLER_PORT, "1337");
        cfg_set_value(agent->config, CFG_CONTROLLER_ADDRESS, "unix::");
        cfg_set_value(agent->config, CFG_HEARTBEAT_INTERVAL, "1000");
        cfg_set_value(agent->config, CFG_TCP_KEEPALIVE_TIME, "10000");
        cfg_set_value(agent->config, CFG_TCP_KEEPALIVE_INTERVAL, "5000");
        cfg_set_value(agent->config, CFG_TCP_KEEPALIVE_COUNT, "3");
        cfg_set_value(agent->config, CFG_IP_RECEIVE_ERRORS, "true");

        bool result = agent_apply_config(agent);
        if (!result) {
                print_error_result(__func__, true, result);
                return false;
        }

        return check_agent(__func__, agent, "node-foo", "127.0.0.1", 1337, "unix::", 1000);
}

bool test_agent_apply_config_invalid_port() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(agent->config, CFG_CONTROLLER_PORT, "invalid");

        bool result = agent_apply_config(agent);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_agent_apply_config_invalid_heartbeat() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(agent->config, CFG_HEARTBEAT_INTERVAL, "invalid");

        bool result = agent_apply_config(agent);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_agent_apply_config_invalid_tcpkeeptime() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(agent->config, CFG_TCP_KEEPALIVE_TIME, "invalid");

        bool result = agent_apply_config(agent);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_agent_apply_config_invalid_tcpkeepintvl() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(agent->config, CFG_TCP_KEEPALIVE_INTERVAL, "invalid");

        bool result = agent_apply_config(agent);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_agent_apply_config_invalid_tcpkeepcnt() {
        _cleanup_agent_ Agent *agent = agent_new();
        int r = cfg_initialize(&agent->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(agent->config, CFG_TCP_KEEPALIVE_COUNT, "invalid");

        bool result = agent_apply_config(agent);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

int main() {
        bool result = true;
        result = result && test_agent_apply_config_none();
        result = result && test_agent_apply_config_valid_all();
        result = result && test_agent_apply_config_invalid_port();
        result = result && test_agent_apply_config_invalid_heartbeat();
        result = result && test_agent_apply_config_invalid_tcpkeeptime();
        result = result && test_agent_apply_config_invalid_tcpkeepintvl();
        result = result && test_agent_apply_config_invalid_tcpkeepcnt();

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
