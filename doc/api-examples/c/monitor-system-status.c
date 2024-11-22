/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: MIT-0
 */
#include <systemd/sd-bus.h>

static int setup_event_loop(sd_bus *bus, sd_event **event) {
        int r = sd_event_default(event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s", strerror(-r));
                return r;
        }

        r = sd_bus_attach_event(bus, *event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach api bus to event: %s", strerror(-r));
                sd_event_unrefp(event);
                return r;
        }

        return 0;
}

static int on_properties_changed(sd_bus_message *m, void *userdata, sd_bus_error *error) {
        (void) sd_bus_message_skip(m, "s");

        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                fprintf(stderr, "Failed to read changed properties: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                if (r <= 0) {
                        break;
                }

                const char *key = NULL;
                r = sd_bus_message_read(m, "s", &key);
                if (r < 0) {
                        fprintf(stderr, "Failed to read next unit changed property: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }

                /* only process property changes for the node status or the controller address */
                if (strcmp(key, "Status") == 0) {
                        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
                        if (r < 0) {
                                fprintf(stderr, "Failed to enter content of variant type: %s\n", strerror(-r));
                                return r;
                        }
                        char *status = NULL;
                        r = sd_bus_message_read(m, "s", &status);
                        if (r < 0) {
                                fprintf(stderr, "Failed to read value of changed property: %s\n", strerror(-r));
                                return r;
                        }

                        r = sd_bus_message_exit_container(m);
                        if (r < 0) {
                                fprintf(stderr, "Failed to exit container: %s\n", strerror(-r));
                                return r;
                        }

                        fprintf(stdout, "System status: %s\n", status);
                }
        }

        return sd_bus_message_exit_container(m);
}

int main(int argc, char *argv[]) {
        int r = 0;
        sd_bus *bus = NULL;

        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s", strerror(-r));
                return r;
        }

        sd_event *event = NULL;
        r = setup_event_loop(bus, &event);
        if (r < 0) {
                sd_bus_unref(bus);
                return 1;
        }

        sd_bus_match_signal(
                        bus,
                        NULL,
                        "org.eclipse.bluechi",
                        "/org/eclipse/bluechi",
                        "org.freedesktop.DBus.Properties",
                        "PropertiesChanged",
                        on_properties_changed,
                        NULL);

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Failed to start event loop: %s", strerror(-r));
                sd_event_unref(event);
                sd_bus_unref(bus);
                return r;
        }
        sd_event_unref(event);
        sd_bus_unref(bus);

        return 0;
}
