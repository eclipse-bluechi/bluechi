/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-daemon-reload.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/opt.h"

static int method_daemon_reload_on(Client *client, char *node_name) {
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
                        "Reload",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        return 0;
}

int method_daemon_reload(Command *command, void *userdata) {
        return method_daemon_reload_on(userdata, command->opargv[0]);
}


void usage_method_daemon_reload() {
        usage_print_header();
        usage_print_description("Reload all units on a node");
        usage_print_usage("bluechictl daemon-reload [nodename]");
}
