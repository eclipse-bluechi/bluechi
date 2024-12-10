/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/common.h"

#include "controller/controller.h"
#include "controller/node.h"

/*
 * custom controller cleanup to remove added nodes since
 * controller_set_config_values extracts node names from
 * ALLOWED_NODE_NAMES and pre-creates nodes
 */
void controller_cleanup(Controller *controller) {
        if (controller) {
                Node *curr = NULL;
                Node *next = NULL;
                LIST_FOREACH_SAFE(nodes, curr, next, controller->nodes) {
                        controller_remove_node(controller, curr);
                }
        }
        controller_unrefp(&controller);
}

DEFINE_CLEANUP_FUNC(Controller, controller_cleanup)
#define _test_cleanup_controller_ _cleanup_(controller_cleanupp)


void print_error_result(const char *func_name, bool expected, bool actual) {
        fprintf(stderr,
                "FAILED: expected %s to return '%s', but got '%s'\n",
                func_name,
                bool_to_str(expected),
                bool_to_str(actual));
}

void print_controller_field_error(
                const char *func_name, const char *field_name, const char *expected, const char *actual) {
        fprintf(stderr,
                "FAILED: expected %s to set 'controller->%s' to '%s', but got '%s'\n",
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
                print_controller_field_error(func_name, field_name, expected, actual);
                return false;
        }
        return true;
}

bool check_controller(
                const char *func_name,
                Controller *controller,
                bool expected_use_tcp,
                bool expected_use_uds,
                uint16_t expected_port,
                const char **expected_node_names,
                uint16_t expected_node_number) {

        bool result = true;

        if (expected_use_tcp != controller->use_tcp) {
                print_controller_field_error(
                                func_name,
                                "use_tcp",
                                bool_to_str(expected_use_tcp),
                                bool_to_str(controller->use_tcp));
                result = false;
        }

        if (expected_use_uds != controller->use_uds) {
                print_controller_field_error(
                                func_name,
                                "use_uds",
                                bool_to_str(expected_use_uds),
                                bool_to_str(controller->use_uds));
                result = false;
        }

        if (expected_port != controller->port) {
                _cleanup_free_ char *actual = NULL;
                int r = asprintf(&actual, "%d", controller->port);
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
                print_controller_field_error(func_name, "port", expected, actual);
                result = false;
        }

        // socket options have private members and can't be evaluated at the moment

        for (uint16_t i = 0; i < expected_node_number; i++) {
                bool found = false;
                Node *curr = NULL;
                Node *next = NULL;
                LIST_FOREACH_SAFE(nodes, curr, next, controller->nodes) {
                        if (streq(expected_node_names[i], curr->name)) {
                                found = true;
                                break;
                        }
                }
                if (!found) {
                        fprintf(stderr,
                                "FAILED: '%s' - didn't find node '%s'\n",
                                func_name,
                                expected_node_names[i]);
                        result = false;
                }
        }


        return result;
}

bool test_controller_apply_config_none() {
        _test_cleanup_controller_ Controller *controller = controller_new();
        int r = cfg_initialize(&controller->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        bool result = controller_apply_config(controller);
        if (!result) {
                print_error_result(__func__, true, result);
                return false;
        }

        return check_controller(__func__, controller, false, false, 0, NULL, 0);
}


bool test_controller_apply_config_valid_all() {
        _test_cleanup_controller_ Controller *controller = controller_new();
        int r = cfg_initialize(&controller->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(controller->config, CFG_CONTROLLER_USE_TCP, "true");
        cfg_set_value(controller->config, CFG_CONTROLLER_USE_UDS, "true");
        cfg_set_value(controller->config, CFG_CONTROLLER_PORT, "1337");
        cfg_set_value(controller->config, CFG_ALLOWED_NODE_NAMES, "foo,bar,another");
        cfg_set_value(controller->config, CFG_TCP_KEEPALIVE_TIME, "10000");
        cfg_set_value(controller->config, CFG_TCP_KEEPALIVE_INTERVAL, "5000");
        cfg_set_value(controller->config, CFG_TCP_KEEPALIVE_COUNT, "3");
        cfg_set_value(controller->config, CFG_IP_RECEIVE_ERRORS, "true");

        bool result = controller_apply_config(controller);
        if (!result) {
                print_error_result(__func__, true, result);
                return false;
        }

        const char *expected_nodes[3] = { "foo", "bar", "another" };
        return check_controller(__func__, controller, true, true, 1337, expected_nodes, 3);
}

bool test_controller_apply_config_invalid_port() {
        _test_cleanup_controller_ Controller *controller = controller_new();
        int r = cfg_initialize(&controller->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(controller->config, CFG_CONTROLLER_PORT, "invalid");

        bool result = controller_apply_config(controller);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_controller_apply_config_invalid_tcpkeeptime() {
        _test_cleanup_controller_ Controller *controller = controller_new();
        int r = cfg_initialize(&controller->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(controller->config, CFG_TCP_KEEPALIVE_TIME, "invalid");

        bool result = controller_apply_config(controller);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_controller_apply_config_invalid_tcpkeepintvl() {
        _test_cleanup_controller_ Controller *controller = controller_new();
        int r = cfg_initialize(&controller->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(controller->config, CFG_TCP_KEEPALIVE_INTERVAL, "invalid");

        bool result = controller_apply_config(controller);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

bool test_controller_apply_config_invalid_tcpkeepcnt() {
        _test_cleanup_controller_ Controller *controller = controller_new();
        int r = cfg_initialize(&controller->config);
        if (r < 0) {
                fprintf(stderr, "Unexpected error when initializing config: %s", strerror(-r));
                return false;
        }

        cfg_set_value(controller->config, CFG_TCP_KEEPALIVE_COUNT, "invalid");

        bool result = controller_apply_config(controller);
        if (result) {
                print_error_result(__func__, false, result);
                return false;
        }
        return true;
}

int main() {
        bool result = true;
        result = result && test_controller_apply_config_none();
        result = result && test_controller_apply_config_valid_all();
        result = result && test_controller_apply_config_invalid_port();
        result = result && test_controller_apply_config_invalid_tcpkeeptime();
        result = result && test_controller_apply_config_invalid_tcpkeepintvl();
        result = result && test_controller_apply_config_invalid_tcpkeepcnt();

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
