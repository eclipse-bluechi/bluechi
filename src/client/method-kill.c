/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-kill.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/opt.h"
#include "libbluechi/common/parse-util.h"


static int method_kill_unit_on(
                Client *client, const char *node_name, const char *unit_name, const char *whom, uint32_t signal) {

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
                        "KillUnit",
                        &error,
                        &result,
                        "ssi",
                        unit_name,
                        whom,
                        signal);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call for KillUnit: %s\n", error.message);
                return r;
        }

        return 0;
}

static const char *default_whom = "all";
static const char *default_signal = "15";

// Based on values listed for KillUnit
// https://www.freedesktop.org/wiki/Software/systemd/dbus/
static const char *whom_values[] = { "all", "control", "main", NULL };

static bool is_whom_value_valid(const char *v) {
        for (int i = 0; whom_values[i]; i++) {
                if (whom_values[i] && streq(whom_values[i], v)) {
                        return true;
                }
        }
        return false;
}

int method_kill(Command *command, UNUSED void *userdata) {
        const char *whom = command_get_option(command, ARG_KILL_WHOM_SHORT);
        if (whom == NULL) {
                whom = default_whom;
        }
        if (!is_whom_value_valid(whom)) {
                fprintf(stderr, "Value '%s' of option --%s is invalid\n", whom, ARG_KILL_WHOM);
                return -EINVAL;
        }

        uint32_t sig = 0;
        const char *signal = command_get_option(command, ARG_SIGNAL_SHORT);
        if (signal == NULL) {
                signal = default_signal;
        }
        if (!parse_linux_signal(signal, &sig)) {
                fprintf(stderr, "Value '%s' of option --%s is invalid\n", signal, ARG_SIGNAL);
                return -EINVAL;
        }

        return method_kill_unit_on(userdata, command->opargv[0], command->opargv[1], whom, sig);
}

void usage_method_kill() {
        usage_print_header();
        usage_print_description("Kill (i.e. send a signal to) the processes of a unit");
        usage_print_usage("bluechictl kill [nodename] [unit] [options]");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
        printf("  --%s \t Enum defining which processes of the unit are killed.\n", ARG_KILL_WHOM);
        printf("\t\t Needs to be one of [all, main, control]. Default: '%s'\n", default_whom);
        printf("  --%s \t The signal sent to kill the processes of the unit. Default: '%s'\n",
               ARG_SIGNAL,
               default_signal);
}
