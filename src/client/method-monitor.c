/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdio.h>

#include "client.h"
#include "method-monitor.h"
#include "usage.h"

#include "libbluechi/common/common.h"
#include "libbluechi/common/list.h"

/***************************************************************
 ******** Monitor: Changes in systemd units of agents **********
 ***************************************************************/

static int on_unit_new_signal(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        const char *node_name = NULL, *unit_name = NULL, *reason = NULL;

        int r = 0;
        r = sd_bus_message_read(m, "sss", &node_name, &unit_name, &reason);
        if (r < 0) {
                fprintf(stderr, "Can't parse unit new signal: %s\n", strerror(-r));
                return 0;
        }

        printf("[%s] %s\n\tUnit created (reason: %s)\n", node_name, unit_name, reason);
        fflush(stdout);

        return 0;
}

static int on_unit_removed_signal(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        const char *node_name = NULL, *unit_name = NULL, *reason = NULL;

        int r = 0;
        r = sd_bus_message_read(m, "sss", &node_name, &unit_name, &reason);
        if (r < 0) {
                fprintf(stderr, "Can't parse unit removed signal: %s\n", strerror(-r));
                return 0;
        }

        printf("[%s] %s\n\tUnit removed (reason: %s)\n", node_name, unit_name, reason);
        fflush(stdout);

        return 0;
}

static int on_unit_state_changed_signal(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        const char *node_name = NULL, *unit_name = NULL, *active_state = NULL, *sub_state = NULL,
                   *reason = NULL;

        int r = 0;
        r = sd_bus_message_read(m, "sssss", &node_name, &unit_name, &active_state, &sub_state, &reason);
        if (r < 0) {
                fprintf(stderr, "Can't parse unit state changed signal: %s\n", strerror(-r));
                return 0;
        }

        printf("[%s] %s\n\tUnit state changed (reason: %s)\n\tActive: %s (%s)\n",
               node_name,
               unit_name,
               reason,
               active_state,
               sub_state);
        fflush(stdout);

        return 0;
}

static int on_unit_properties_changed_signal(
                sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        const char *node_name = NULL, *unit_name = NULL, *interface_name = NULL;

        int r = 0;
        r = sd_bus_message_read(m, "sss", &node_name, &unit_name, &interface_name);
        if (r < 0) {
                fprintf(stderr, "Can't parse unit properties changed signal: %s\n", strerror(-r));
                return 0;
        }

        printf("[%s] %s\n\tUnit properties changed (Interface: %s)\n", node_name, unit_name, interface_name);
        fflush(stdout);

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                fprintf(stderr, "Failed to read properties: %s\n", strerror(-r));
                return r;
        }


        for (;;) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                if (r <= 0) {
                        break;
                }

                const char *s = NULL;
                r = sd_bus_message_read(m, "s", &s);
                if (r < 0) {
                        fprintf(stderr, "Failed to read next unit changed property: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }

                const char *contents = NULL;
                r = sd_bus_message_peek_type(m, NULL, &contents);
                if (r < 0) {
                        fprintf(stderr, "Failed to determine content of variant type: %s", strerror(-r));
                        return r;
                }

                if (streq(contents, "s")) {
                        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, contents);
                        if (r < 0) {
                                fprintf(stderr, "Failed to enter content of variant type: %s", strerror(-r));
                                return r;
                        }
                        const char *value = NULL;
                        r = sd_bus_message_read(m, "s", &value);
                        if (r < 0) {
                                fprintf(stderr, "Failed to read value of changed property: %s\n", strerror(-r));
                                return r;
                        }
                        fprintf(stdout, "\t%s: %s\n", s, value);
                } else {
                        (void) sd_bus_message_skip(m, "v");
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit container: %s\n", strerror(-r));
                        return r;
                }
        }

        return sd_bus_message_exit_container(m);
}

/* units are comma separated */
static int method_monitor_units_on_nodes(Client *client, char *node, char *units) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        char *monitor_path = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "CreateMonitor",
                        &error,
                        &reply,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to create monitor: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_read(reply, "o", &monitor_path);
        if (r < 0) {
                fprintf(stderr, "Failed to parse create monitor response message: %s\n", strerror(-r));
                return r;
        }

        printf("Monitor path: %s\n", monitor_path);

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor_path,
                        MONITOR_INTERFACE,
                        "UnitNew",
                        on_unit_new_signal,
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitNew: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor_path,
                        MONITOR_INTERFACE,
                        "UnitRemoved",
                        on_unit_removed_signal,
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitRemoved: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor_path,
                        MONITOR_INTERFACE,
                        "UnitStateChanged",
                        on_unit_state_changed_signal,
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitStateChanged: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor_path,
                        MONITOR_INTERFACE,
                        "UnitPropertiesChanged",
                        on_unit_properties_changed_signal,
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for UnitPropertiesChanged: %s\n", strerror(-r));
                return r;
        }

        _cleanup_sd_bus_message_ sd_bus_message *m = NULL;
        r = sd_bus_message_new_method_call(
                        client->api_bus,
                        &m,
                        BC_INTERFACE_BASE_NAME,
                        monitor_path,
                        MONITOR_INTERFACE,
                        "SubscribeList");
        if (r < 0) {
                fprintf(stderr, "Failed creating subscription call: %s\n", strerror(-r));
                return r;
        }

        sd_bus_message_append(m, "s", node);

        r = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, "s");
        if (r < 0) {
                fprintf(stderr, "Failed opening subscription unit array: %s\n", strerror(-r));
                return r;
        }


        char *unit_saveptr = NULL;
        char *unit = strtok_r(units, ",", &unit_saveptr);
        for (;;) {
                if (unit == NULL) {
                        break;
                }

                fprintf(stdout, "Subscribing to node '%s' and unit '%s'\n", node, unit);
                sd_bus_message_append(m, "s", unit);

                unit = strtok_r(NULL, ",", &unit_saveptr);
        }

        r = sd_bus_message_close_container(m);
        if (r < 0) {
                fprintf(stderr, "Failed closing subscription unit array: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, m, BC_DEFAULT_DBUS_TIMEOUT, &error, &reply);
        if (r < 0) {
                fprintf(stderr, "Failed to subscribe to monitor: %s\n", error.message);
                return r;
        }

        return client_start_event_loop(client);
}

int method_monitor(Command *command, void *userdata) {
        char *arg0 = SYMBOL_WILDCARD;
        char *arg1 = SYMBOL_WILDCARD;
        switch (command->opargc) {
        case 0:
                break;
        case 1:
                arg0 = command->opargv[0];
                break;
        case 2:
                arg0 = command->opargv[0];
                arg1 = command->opargv[1];
                break;
        default:
                return -EINVAL;
        }
        return method_monitor_units_on_nodes(userdata, arg0, arg1);
}

void usage_method_monitor() {
        usage_print_header();
        usage_print_description("Monitor changes in the BlueChi managed system");
        usage_print_usage("bluechictl monitor [nodename] [unitname]");
        printf("  If [nodename] and [unitname] are not given, changes on all nodes for all units will be monitored.\n");
        printf("  If a wildcard '*' is used for [nodename] and/or [unitname], all nodes and/or units are monitored.\n");
        printf("\n");
        printf("Examples:\n");
        printf("  bluechictl monitor\n");
        printf("  bluechictl monitor primary \\*\n");
        printf("  bluechictl monitor \\* interesting.service\n");
        printf("  bluechictl monitor primary interesting.service\n");
}
