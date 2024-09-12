/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-unit-lifecycle.h"
#include "client.h"
#include "usage.h"

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

void usage_method_lifecycle() {
        usage_print_header();
        usage_print_description("Start/Stop/Restart/Reload a unit on a node");
        usage_print_usage("bluechictl [start|stop|restart|reload] [nodename] [unitname]");
        printf("\n");
        printf("Examples:\n");
        printf("  bluechictl start primary interesting.service\n");
        printf("  bluechictl stop primary interesting.service\n");
        printf("  bluechictl restart primary interesting.service\n");
        printf("  bluechictl reload primary interesting.service\n");
}
