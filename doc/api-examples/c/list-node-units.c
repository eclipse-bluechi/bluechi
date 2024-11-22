/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <systemd/sd-bus.h>

static int get_node_path(sd_bus *bus, const char *node_name, char **ret_node_path) {
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
        int r = 0;

        if (argc != 2) {
                fprintf(stdout, "Usage: %s node_name\n", argv[0]);
                return 1;
        }

        const char *node_name = argv[1];

        sd_bus *bus = NULL;
        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s", strerror(-r));
                return r;
        }

        char *node_path = NULL;
        r = get_node_path(bus, node_name, &node_path);
        if (r < 0) {
                fprintf(stderr, "Failed to get node path: %s\n", strerror(-r));
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
                        "ListUnits",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                sd_bus_unref(bus);
                return r;
        }

        r = sd_bus_message_enter_container(result, SD_BUS_TYPE_ARRAY, "(ssssssouso)");
        if (r < 0) {
                fprintf(stderr, "Failed to read sd-bus message: %s\n", strerror(-r));
                sd_bus_unref(bus);
                sd_bus_message_unref(result);
                sd_bus_error_free(&error);
                return r;
        }

        for (;;) {
                char *node = NULL;
                char *id = NULL;

                r = sd_bus_message_read(
                                result, "(ssssssouso)", &node, &id, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to read unit info: %s\n", strerror(-r));
                        sd_bus_unref(bus);
                        sd_bus_message_unref(result);
                        sd_bus_error_free(&error);
                        return r;
                }
                if (r == 0) {
                        break;
                }

                fprintf(stdout, "%s - %s\n", node, id);
        }

        fprintf(stdout, "\n");

        sd_bus_unref(bus);
        sd_bus_message_unref(result);
        sd_bus_error_free(&error);

        return r;
}
