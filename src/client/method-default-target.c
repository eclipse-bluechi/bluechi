/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-default-target.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/opt.h"

static int method_get_default_target_on(Client *client, char *node_name) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        char *result = NULL;
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
                        "GetDefaultTarget",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_read(message, "s", &result);
        if (r < 0) {
                fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
                return r;
        }

        printf("%s\n", result);

        return r;
}


static int method_set_default_target_on(Client *client, char *node_name, char *target, int force) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
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
                        "SetDefaultTarget",
                        &error,
                        &message,
                        "sb",
                        target,
                        force);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, "(sss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open the strings array container: %s\n", strerror(-r));
                return r;
        }
        printf("[");
        for (;;) {
                const char *result1 = NULL;
                const char *result2 = NULL;
                const char *result3 = NULL;

                r = sd_bus_message_read(message, "(sss)", &result1, &result2, &result3);
                if (r < 0) {
                        fprintf(stderr, "Failed to read the SetDefaultTarget response: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                printf("(%s %s %s),", result1, result2, result3);
        }
        printf("]\n");

        return r;
}

void usage_method_get_default_target() {
        usage_print_header();
        usage_print_usage("bluechictl get-default [nodename] [options]");
        printf("\n");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
}

void usage_method_set_default_target() {
        usage_print_header();
        usage_print_usage("bluechictl set-default [nodename] [target] [options]");
        printf("\n");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
}


int method_get_default_target(Command *command, void *userdata) {
        return method_get_default_target_on(userdata, command->opargv[0]);
}

static const char *glob_systemd_target = "*target";

int method_set_default_target(Command *command, void *userdata) {
        if (!match_glob(command->opargv[1], glob_systemd_target)) {
                fprintf(stderr, "Error: Unit '%s' is not a target unit\n", command->opargv[1]);
                return 1;
        }

        // mimicking systemctl here and setting the force parameter to 1
        // https://github.com/systemd/systemd/blob/7c52d5236a3bc85db1755de6a458934be095cd1c/src/systemctl/systemctl-set-default.c#L130
        return method_set_default_target_on(userdata, command->opargv[0], command->opargv[1], 1);
}
