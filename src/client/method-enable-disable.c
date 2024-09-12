/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-enable-disable.h"
#include "client.h"
#include "method-daemon-reload.h"
#include "usage.h"

#include "libbluechi/common/opt.h"

static int add_string_array_to_message(sd_bus_message *m, char **units, size_t units_count) {
        _cleanup_free_ char **units_array = NULL;
        int r = 0;

        units_array = malloc0_array(0, sizeof(char *), units_count + 1);
        if (units_array == NULL) {
                fprintf(stderr, "Failed to allocate memory\n");
                return -ENOMEM;
        }
        size_t i = 0;
        for (i = 0; i < units_count; i++) {
                units_array[i] = units[i];
        }

        r = sd_bus_message_append_strv(m, units_array);
        if (r < 0) {
                fprintf(stderr, "Failed to append the list of units to the message: %s\n", strerror(-r));
                return r;
        }

        return 0;
}

static int parse_enable_disable_response_from_message(sd_bus_message *m) {
        int r = 0;

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(sss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open the strings array container: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                const char *op_type = NULL;
                const char *symlink_file = NULL;
                const char *symlink_dest = NULL;

                r = sd_bus_message_read(m, "(sss)", &op_type, &symlink_file, &symlink_dest);
                if (r < 0) {
                        fprintf(stderr, "Failed to read enabled unit file information: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                if (streq(op_type, "symlink")) {
                        fprintf(stderr, "Created symlink %s -> %s\n", symlink_file, symlink_dest);
                } else if (streq(op_type, "unlink")) {
                        fprintf(stderr, "Removed \"%s\".\n", symlink_file);
                } else {
                        fprintf(stderr, "Unknown operation: %s\n", op_type);
                }
        }

        return 0;
}

static int method_enable_unit_on(
                Client *client, char *node_name, char **units, size_t units_count, int runtime, int force) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;

        r = client_create_message_new_method_call(client, node_name, "EnableUnitFiles", &outgoing_message);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new message: %s\n", strerror(-r));
                return r;
        }

        r = add_string_array_to_message(outgoing_message, units, units_count);
        if (r < 0) {
                fprintf(stderr, "Failed to append the string array to the message: %s\n", strerror(-r));
        }

        r = sd_bus_message_append(outgoing_message, "bb", runtime, force);
        if (r < 0) {
                fprintf(stderr, "Failed to append runtime and force to the message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, outgoing_message, BC_DEFAULT_DBUS_TIMEOUT, &error, &result);
        if (r < 0) {
                fprintf(stderr, "Failed to issue call: %s\n", error.message);
                return r;
        }

        int carries_install_info = 0;
        r = sd_bus_message_read(result, "b", &carries_install_info);
        if (r < 0) {
                fprintf(stderr, "Failed to read carries_install_info from the message: %s\n", strerror(-r));
                return r;
        }

        if (carries_install_info) {
                fprintf(stderr, "The unit files included enablement information\n");
        } else {
                fprintf(stderr, "The unit files did not include any enablement information\n");
        }

        r = parse_enable_disable_response_from_message(result);
        if (r < 0) {
                fprintf(stderr, "Failed to parse the response strings array: %s\n", error.message);
                return r;
        }

        return 0;
}

static int method_disable_unit_on(Client *client, char *node_name, char **units, size_t units_count, int runtime) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;

        r = client_create_message_new_method_call(client, node_name, "DisableUnitFiles", &outgoing_message);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new message: %s\n", strerror(-r));
                return r;
        }

        r = add_string_array_to_message(outgoing_message, units, units_count);
        if (r < 0) {
                fprintf(stderr, "Failed to append the string array to the message: %s\n", strerror(-r));
        }

        r = sd_bus_message_append(outgoing_message, "b", runtime);
        if (r < 0) {
                fprintf(stderr, "Failed to append runtime to the message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, outgoing_message, BC_DEFAULT_DBUS_TIMEOUT, &error, &result);
        if (r < 0) {
                fprintf(stderr, "Failed to issue call: %s\n", error.message);
                return r;
        }

        r = parse_enable_disable_response_from_message(result);
        if (r < 0) {
                fprintf(stderr, "Failed to parse the response strings array: %s\n", error.message);
                return r;
        }

        return 0;
}

int method_enable(Command *command, void *userdata) {
        int r = 0;
        r = method_enable_unit_on(
                        userdata,
                        command->opargv[0],
                        &command->opargv[1],
                        command->opargc - 1,
                        command_flag_exists(command, ARG_RUNTIME_SHORT),
                        command_flag_exists(command, ARG_FORCE_SHORT));
        if (r < 0) {
                fprintf(stderr,
                        "Failed to enable the units on node [%s] - %s",
                        command->opargv[0],
                        strerror(-r));
                return r;
        }

        if (!command_flag_exists(command, ARG_NO_RELOAD_SHORT)) {
                r = method_daemon_reload(command, userdata);
        }

        return r;
}

int method_disable(Command *command, void *userdata) {
        int r = 0;
        r = method_disable_unit_on(
                        userdata,
                        command->opargv[0],
                        &command->opargv[1],
                        command->opargc - 1,
                        command_flag_exists(command, ARG_RUNTIME_SHORT));
        if (r < 0) {
                fprintf(stderr,
                        "Failed to disable the units on node [%s] - %s",
                        command->opargv[0],
                        strerror(-r));
        }
        if (!command_flag_exists(command, ARG_NO_RELOAD_SHORT)) {
                r = method_daemon_reload(command, userdata);
        }

        return r;
}


void usage_method_enable() {
        usage_print_header();
        usage_print_description("Enable a unit on a node");
        usage_print_usage("bluechictl enable [nodename] [unit1, unit2, ...] [options]");
        printf("\n");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
        printf("  --%s \t if set, the changes don't persist after reboot \n", ARG_RUNTIME);
        printf("  --%s \t if set, symlinks pointing to other units are replaced \n", ARG_FORCE);
        printf("  --%s \t if set, no daemon-reload is triggered \n", ARG_NO_RELOAD);
}

void usage_method_disable() {
        usage_print_header();
        usage_print_description("Disable a unit on a node");
        usage_print_usage("bluechictl disable [nodename] [unit1, unit2, ...] [options]");
        printf("\n");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
        printf("  --%s \t if set, the changes don't persist after reboot \n", ARG_RUNTIME);
        printf("  --%s \t if set, no daemon-reload is triggered \n", ARG_NO_RELOAD);
}
