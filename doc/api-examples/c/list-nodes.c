/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <stdbool.h>
#include <systemd/sd-bus.h>

int main() {
        int r = 0;
        sd_bus *bus = NULL;

        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s", strerror(-r));
                return r;
        }

        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *result = NULL;

        r = sd_bus_call_method(
                        bus,
                        "org.eclipse.bluechi",
                        "/org/eclipse/bluechi",
                        "org.eclipse.bluechi.Controller",
                        "ListNodes",
                        &error,
                        &result,
                        "",
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                sd_bus_unref(bus);
                return r;
        }

        r = sd_bus_message_enter_container(result, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open result array: %s\n", strerror(-r));
                sd_bus_unref(bus);
                sd_bus_message_unref(result);
                sd_bus_error_free(&error);
                return r;
        }
        while (sd_bus_message_at_end(result, false) == 0) {
                const char *name = NULL;
                const char *path = NULL;
                const char *state = NULL;
                const char *ip = NULL;

                r = sd_bus_message_read(result, "(soss)", &name, &path, &state, &ip);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        sd_bus_unref(bus);
                        sd_bus_message_unref(result);
                        sd_bus_error_free(&error);
                        return r;
                }

                fprintf(stdout, "Node: %s, Status: %s\n", name, state);
        }
        fprintf(stdout, "\n");

        sd_bus_unref(bus);
        sd_bus_message_unref(result);
        sd_bus_error_free(&error);

        return 0;
}
