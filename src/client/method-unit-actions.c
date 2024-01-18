/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "method-unit-actions.h"
#include "client.h"

#include "libbluechi/common/opt.h"

static int method_lifecycle_action_on(Client *client, char *node_name, char *unit, char *method) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *job_result = NULL;
        char *job_path = NULL, *result = NULL, *node = NULL;
        uint32_t id = 0;
        int r = 0;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        BC_CONTROLLER_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "JobRemoved",
                        match_job_removed_signal,
                        client);

        if (r < 0) {
                fprintf(stderr, "Failed to match signal\n");
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        method,
                        &error,
                        &message,
                        "ss",
                        unit,
                        "replace");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_read(message, "o", &job_path);
        if (r < 0) {
                fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
                return r;
        }

        job_result = client_wait_for_job(client, job_path);
        if (job_result == NULL) {
                return -EIO;
        }

        r = sd_bus_message_read(job_result, "uosss", &id, &job_path, &node, &unit, &result);
        if (r < 0) {
                fprintf(stderr, "Can't parse job result\n");
                return r;
        }

        printf("Unit %s %s operation result: %s\n", unit, method, result);

        return r;
}

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

int method_start(Command *command, void *userdata) {
        return method_lifecycle_action_on(userdata, command->opargv[0], command->opargv[1], "StartUnit");
}

int method_stop(Command *command, void *userdata) {
        return method_lifecycle_action_on(userdata, command->opargv[0], command->opargv[1], "StopUnit");
}

int method_restart(Command *command, void *userdata) {
        return method_lifecycle_action_on(userdata, command->opargv[0], command->opargv[1], "RestartUnit");
}

int method_reload(Command *command, void *userdata) {
        return method_lifecycle_action_on(userdata, command->opargv[0], command->opargv[1], "ReloadUnit");
}

int method_freeze(Command *command, void *userdata) {
        return method_freeze_unit_on(userdata, command->opargv[0], command->opargv[1]);
}

int method_thaw(Command *command, void *userdata) {
        return method_thaw_unit_on(userdata, command->opargv[0], command->opargv[1]);
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
                r = method_daemon_reload_on(userdata, command->opargv[0]);
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
                r = method_daemon_reload_on(userdata, command->opargv[0]);
        }

        return r;
}

int method_daemon_reload(Command *command, void *userdata) {
        return method_daemon_reload_on(userdata, command->opargv[0]);
}
