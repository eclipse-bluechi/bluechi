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

static int on_unit_new(sd_bus_message *m, void *userdata, sd_bus_error *error) {
        const char *node_name = (char *) userdata;

        char *unit, *reason = NULL;
        int r = sd_bus_message_read(m, "ss", &unit, &reason);
        if (r < 0) {
                fprintf(stderr, "Failed to read unit new signal: %s\n", strerror(-r));
                return r;
        }
        fprintf(stdout, "New Unit %s on node %s, reason: %s\n", unit, node_name, reason);

        return 0;
}

static int on_unit_removed(sd_bus_message *m, void *userdata, sd_bus_error *error) {
        const char *node_name = (char *) userdata;

        char *unit, *reason = NULL;
        int r = sd_bus_message_read(m, "ss", &unit, &reason);
        if (r < 0) {
                fprintf(stderr, "Failed to read unit removed signal: %s\n", strerror(-r));
                return r;
        }
        fprintf(stdout, "Removed Unit %s on node %s, reason: %s\n", unit, node_name, reason);

        return 0;
}

static int on_unit_props_changed(sd_bus_message *m, void *userdata, sd_bus_error *error) {
        const char *node_name = (char *) userdata;

        char *unit, *iface = NULL;
        int r = sd_bus_message_read(m, "ss", &unit, &iface);
        if (r < 0) {
                fprintf(stderr, "Failed to read unit props changed signal: %s\n", strerror(-r));
                return r;
        }
        fprintf(stdout, "Unit %s on node %s changed for iface %s\n", unit, node_name, iface);

        return 0;
}

static int on_unit_state_changed(sd_bus_message *m, void *userdata, sd_bus_error *error) {
        const char *node_name = (char *) userdata;

        char *unit, *active_state, *sub_state, *reason = NULL;
        int r = sd_bus_message_read(m, "ssss", &unit, &active_state, &sub_state, &reason);
        if (r < 0) {
                fprintf(stderr, "Failed to read unit state changed signal: %s\n", strerror(-r));
                return r;
        }
        fprintf(stdout,
                "Unit %s on node %s changed to state (%s, %s), reason: %s\n",
                unit,
                node_name,
                active_state,
                sub_state,
                reason);

        return 0;
}

static int create_monitor(sd_bus *bus, char **ret_monitor_path) {
        int r = 0;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message *result = NULL;

        r = sd_bus_call_method(
                        bus,
                        "org.eclipse.bluechi",
                        "/org/eclipse/bluechi",
                        "org.eclipse.bluechi.Controller",
                        "CreateMonitor",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to create monitor: %s\n", error.message);
                return r;
        }

        char *monitor_path = NULL;
        r = sd_bus_message_read(result, "o", &monitor_path);
        if (r < 0) {
                fprintf(stderr, "Failed to parse create monitor response message: %s\n", strerror(-r));
                sd_bus_message_unref(result);
                sd_bus_error_free(&error);
                return r;
        }
        *ret_monitor_path = monitor_path;

        sd_bus_message_unref(result);
        sd_bus_error_free(&error);
        return 0;
}

static int add_unit_listener(sd_bus *bus, const char *monitor_path, char *node_name) {
        int r = 0;

        r = sd_bus_match_signal(
                        bus,
                        NULL,
                        "org.eclipse.bluechi",
                        monitor_path,
                        "org.eclipse.bluechi.Monitor",
                        "UnitNew",
                        on_unit_new,
                        node_name);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitNew: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        bus,
                        NULL,
                        "org.eclipse.bluechi",
                        monitor_path,
                        "org.eclipse.bluechi.Monitor",
                        "UnitRemoved",
                        on_unit_removed,
                        node_name);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitRemoved: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        bus,
                        NULL,
                        "org.eclipse.bluechi",
                        monitor_path,
                        "org.eclipse.bluechi.Monitor",
                        "UnitPropertiesChanged",
                        on_unit_props_changed,
                        node_name);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitPropertiesChanged: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        bus,
                        NULL,
                        "org.eclipse.bluechi",
                        monitor_path,
                        "org.eclipse.bluechi.Monitor",
                        "UnitStateChanged",
                        on_unit_state_changed,
                        node_name);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitStateChanged: %s\n", strerror(-r));
                return r;
        }

        return 0;
}

int main(int argc, char *argv[]) {
        int r = 0;

        if (argc != 2) {
                fprintf(stdout, "Usage: %s node_name\n", argv[0]);
                return 1;
        }
        char *node_name = argv[1];

        sd_bus *bus = NULL;
        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s", strerror(-r));
                return r;
        }

        char *monitor_path = NULL;
        r = create_monitor(bus, &monitor_path);
        if (r < 0) {
                sd_bus_unref(bus);
                return r;
        }

        r = add_unit_listener(bus, monitor_path, node_name);
        if (r < 0) {
                sd_bus_unref(bus);
                return r;
        }

        r = sd_bus_call_method(
                        bus,
                        "org.eclipse.bluechi",
                        monitor_path,
                        "org.eclipse.bluechi.Monitor",
                        "Subscribe",
                        NULL,
                        NULL,
                        "ss",
                        node_name,
                        "*");
        if (r < 0) {
                fprintf(stderr, "Failed to create subscription: %s\n", strerror(-r));
                sd_bus_unref(bus);
                return r;
        }

        sd_event *event = NULL;
        r = setup_event_loop(bus, &event);
        if (r < 0) {
                sd_bus_unref(bus);
                return 1;
        }

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
