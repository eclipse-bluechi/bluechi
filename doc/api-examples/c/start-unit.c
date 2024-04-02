/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <stdbool.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

static int get_node_path(sd_bus *bus, char *node_name, char **ret_node_path) {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *result = NULL;

        int r = sd_bus_call_method(
                        bus,
                        "org.eclipse.bluechi",
                        "/org/eclipse/bluechi",
                        "org.eclipse.bluechi.Controller",
                        "GetNode",
                        &error,
                        &result,
                        "s",
                        node_name);
        if (r < 0) {
                fprintf(stderr, "Failed to resolve node path: %s\n", error.message);
                sd_bus_message_unref(result);
                sd_bus_error_free(&error);
                return r;
        }

        r = sd_bus_message_read(result, "o", ret_node_path);
        if (r < 0) {
                fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
        }

        sd_bus_message_unref(result);
        sd_bus_error_free(&error);
        return r;
}

int main(int argc, char *argv[]) {

        if (argc != 3) {
                fprintf(stderr, "Usage: %s node_name unit_name", argv[0]);
                return EXIT_FAILURE;
        }

        char *node_name = argv[1];
        char *unit_name = argv[2];

        int r = 0;
        sd_bus *bus = NULL;

        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s", strerror(-r));
                return r;
        }

        char *node_path = NULL;
        r = get_node_path(bus, node_name, &node_path);
        if (r < 0) {
                sd_bus_unref(bus);
                return r;
        }

        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *result = NULL;

        r = sd_bus_call_method(
                        bus,
                        "org.eclipse.bluechi",
                        node_path,
                        "org.eclipse.bluechi.Node",
                        "StartUnit",
                        &error,
                        &result,
                        "ss",
                        unit_name,
                        "replace");
        if (r < 0) {
                fprintf(stderr, "Failed to start unit: %s\n", error.message);
                sd_bus_unref(bus);
                return r;
        }

        char *job_path = NULL;
        r = sd_bus_message_read(result, "o", &job_path);
        if (r < 0) {
                fprintf(stderr, "Failed to parse start unit response message: %s\n", strerror(-r));
        }

        fprintf(stdout, "Started unit '%s' on node '%s': %s\n", unit_name, node_name, job_path);

        sd_bus_unref(bus);
        sd_bus_message_unref(result);
        sd_bus_error_free(&error);

        return EXIT_SUCCESS;
}
