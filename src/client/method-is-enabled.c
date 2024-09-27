/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdlib.h>

#include "client.h"
#include "method-is-enabled.h"
#include "usage.h"

#include "libbluechi/common/opt.h"
#include "libbluechi/common/string-util.h"

static int method_is_enabled_on(Client *client, char *node_name, char *unit_file) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        const char *state = NULL;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "GetUnitFileState",
                        &error,
                        &result,
                        "s",
                        unit_file);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_read(result, "s", &state);
        if (r < 0) {
                fprintf(stderr, "Failed to read result of method call: %s\n", error.message);
                return r;
        }

        printf("%s\n", state);

        if (streq(state, "enabled") || streq(state, "enabled-runtime") || streq(state, "static") ||
            streq(state, "alias") || streq(state, "indirect") || streq(state, "generated")) {
                return 0;
        }

        return -1;
}

int method_is_enabled(Command *command, void *userdata) {
        return method_is_enabled_on(userdata, command->opargv[0], command->opargv[1]);
}

void usage_method_is_enabled() {
        usage_print_header();
        usage_print_description("Check whether unit file is enabled");
        usage_print_usage("bluechictl is-enabled [nodename] [unit]");
}
