/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-freeze-thaw.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/opt.h"

static int method_freeze_unit_on(Client *client, char *node_name, char *unit) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "FreezeUnit",
                        &error,
                        &result,
                        "s",
                        unit);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        printf("Unit %s freeze operation done\n", unit);

        return 0;
}

static int method_thaw_unit_on(Client *client, char *node_name, char *unit) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "ThawUnit",
                        &error,
                        &result,
                        "s",
                        unit);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        printf("Unit %s thaw operation done\n", unit);

        return 0;
}

int method_freeze(Command *command, void *userdata) {
        return method_freeze_unit_on(userdata, command->opargv[0], command->opargv[1]);
}

int method_thaw(Command *command, void *userdata) {
        return method_thaw_unit_on(userdata, command->opargv[0], command->opargv[1]);
}


void usage_method_freeze() {
        usage_print_header();
        usage_print_description("Freeze a unit on a node");
        usage_print_usage("bluechictl freeze [nodename] [unit]");
}

void usage_method_thaw() {
        usage_print_header();
        usage_print_description("Thaw a previously frozen unit on a node");
        usage_print_usage("bluechictl freeze [nodename] [unit]");
}
