/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-loglevel.h"
#include "client.h"
#include "usage.h"

static int method_set_loglevel_on(Client *client, char *node_name, char *loglevel) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        if (node_name != NULL) {
                char *path = NULL;
                int r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &path);
                if (r < 0) {
                        r = assemble_object_path_string(
                                        NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
                        if (r < 0) {
                                fprintf(stderr, "Failed to assemble pbject path to string: %s\n", strerror(-r));
                                return r;
                        }
                }

                r = sd_bus_call_method(
                                client->api_bus,
                                BC_INTERFACE_BASE_NAME,
                                path,
                                NODE_INTERFACE,
                                "SetLogLevel",
                                &error,
                                &result,
                                "s",
                                loglevel);
                if (r < 0) {
                        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                        return r;
                }
                return 0;
        } else {
                r = sd_bus_call_method(
                                client->api_bus,
                                BC_INTERFACE_BASE_NAME,
                                BC_OBJECT_PATH,
                                CONTROLLER_INTERFACE,
                                "SetLogLevel",
                                &error,
                                &result,
                                "s",
                                loglevel);
                if (r < 0) {
                        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                        return r;
                }
        }
        return 0;
}

int method_set_loglevel(Command *command, void *userdata) {
        if (command->opargc == 1) {
                return method_set_loglevel_on(userdata, NULL, command->opargv[0]);
        }
        return method_set_loglevel_on(userdata, command->opargv[0], command->opargv[1]);
}

void usage_method_set_loglevel() {
        usage_print_header();
        usage_print_description("Set the LogLevel of BlueChi");
        usage_print_usage("bluechictl set-loglevel [nodename] [loglevel]");
        printf("  If [nodename] is not given, the [loglevel] will be set for the bluechi-controller.\n");
        printf("  [loglevel] has to be one of [DEBUG, INFO, WARN, ERROR].\n");
        printf("\n");
        printf("Examples:\n");
        printf("  bluechictl set-loglevel INFO\n");
        printf("  bluechictl set-loglevel primary DEBUG\n");
}
