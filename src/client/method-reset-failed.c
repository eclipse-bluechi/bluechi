/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-reset-failed.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/opt.h"

static int method_reset_failed_on_units(Client *client, Command *commands) {
        char *node_name = commands->opargv[0];
        int units_count = 1;
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        while (commands->opargv[units_count] != NULL) {
                printf("%i %s\n", units_count, commands->opargv[units_count]);
                r = sd_bus_call_method(
                                client->api_bus,
                                BC_INTERFACE_BASE_NAME,
                                client->object_path,
                                NODE_INTERFACE,
                                "ResetFailedUnit",
                                &error,
                                &result,
                                "s",
                                commands->opargv[units_count]);

                if (r < 0) {
                        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                        return r;
                }
                units_count++;
        }
        return r;
}

static int method_reset_failed_on_node(Client *client, char *node_name) {

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        int r = 0;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "ResetFailed",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr,
                        "Couldn't reset failed state of all units on node '%s': %s\n",
                        node_name,
                        error.message);
                return r;
        }

        return r;
}

static int method_reset_failed_on_all_nodes(Client *client) {

        int r = 0;

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "ListNodes",
                        &error,
                        &result,
                        "",
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(result, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open result array: %s\n", strerror(-r));
                return r;
        }
        while (sd_bus_message_at_end(result, false) == 0) {
                const char *name = NULL;

                r = sd_bus_message_read(result, "(soss)", &name, NULL, NULL, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }

                _cleanup_free_ char *node_name = strdup(name);
                r = method_reset_failed_on_node(client, node_name);
        }

        return r;
}

int method_reset_failed(Command *command, void *userdata) {
        if (command->opargv[0] != NULL) {
                if (command->opargv[1] != NULL) {
                        return method_reset_failed_on_units(userdata, command);
                }
                return method_reset_failed_on_node(userdata, command->opargv[0]);
        }
        return method_reset_failed_on_all_nodes(userdata);
}

void usage_method_reset_failed() {
        usage_print_header();
        usage_print_description("Reset the failed state of units");
        usage_print_usage("bluechictl reset-failed [nodename] [unit1, unit2, ...]");
        printf("  If [nodename] and unit(s) are not given, the failed state is reset for all units on all nodes.\n");
        printf("  If no unit(s) are given, the failed state is reset for all units on the given node.\n");
}
