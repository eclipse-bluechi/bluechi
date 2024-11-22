/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
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

bool parse_uint64(const char *in, uint64_t *ret) {
        const int base = 10;
        char *x = NULL;

        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        errno = 0;
        long r = strtol(in, &x, base);
        if (errno > 0 || !x || x == in || *x != 0) {
                return false;
        }
        *ret = r;

        return true;
}

int main(int argc, char *argv[]) {
        int r = 0;

        if (argc != 4) {
                fprintf(stdout, "Usage: %s node_name unit_name cpu_value\n", argv[0]);
                return 1;
        }

        const char *node_name = argv[1];
        const char *unit_name = argv[2];
        uint64_t cpu_weight = 0;
        if (!parse_uint64(argv[3], &cpu_weight)) {
                fprintf(stderr, "Failed to parse '%s' to uint64\n", argv[3]);
                return 1;
        }

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
                        "SetUnitProperties",
                        &error,
                        &result,
                        "sba(sv)",
                        unit_name,
                        0,
                        1,
                        "CPUWeight",
                        "t",
                        cpu_weight);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                sd_bus_unref(bus);
                return r;
        }

        sd_bus_unref(bus);
        sd_bus_message_unref(result);
        sd_bus_error_free(&error);

        return r;
}
