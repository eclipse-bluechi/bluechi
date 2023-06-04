/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "libhirte/common/common.h"
#include "libhirte/common/list.h"
#include "libhirte/service/shutdown.h"

#include "method_monitor.h"

static int start_event_loop(sd_bus *api_bus) {
        _cleanup_sd_event_ sd_event *event = NULL;
        int r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s", strerror(-r));
                return r;
        }

        r = sd_bus_attach_event(api_bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach api bus to event: %s", strerror(-r));
                return r;
        }

        r = event_loop_add_shutdown_signals(event);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to agent event loop: %s", strerror(-r));
                return r;
        }

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Failed to start event loop: %s", strerror(-r));
                return r;
        }

        return 0;
}

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
int method_monitor_units_on_nodes(sd_bus *api_bus, char *node, char *units) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        char *monitor_path = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        api_bus,
                        HIRTE_INTERFACE_BASE_NAME,
                        HIRTE_OBJECT_PATH,
                        MANAGER_INTERFACE,
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
                        api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
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
                        api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
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
                        api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
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
                        api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
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
                        api_bus, &m, HIRTE_INTERFACE_BASE_NAME, monitor_path, MONITOR_INTERFACE, "SubscribeList");
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

        r = sd_bus_call(api_bus, m, HIRTE_DEFAULT_DBUS_TIMEOUT, &error, &reply);
        if (r < 0) {
                fprintf(stderr, "Failed to subscribe to monitor: %s\n", error.message);
                return r;
        }

        return start_event_loop(api_bus);
}


/***************************************************************
 ******** Monitor: Connection state changes of agents **********
 ***************************************************************/

typedef struct Nodes Nodes;
typedef struct NodeConnection NodeConnection;

struct NodeConnection {
        char *name;
        char *state;
        char *heartbeat_timestamp;
        LIST_FIELDS(NodeConnection, nodes);
};

typedef struct Nodes {
        LIST_HEAD(NodeConnection, nodes);
} Nodes;

static Nodes *nodes_new() {
        Nodes *nodes = malloc0(sizeof(Nodes));
        LIST_HEAD_INIT(nodes->nodes);
        return nodes;
}

static void nodes_add_connection(
                Nodes *nodes,
                const char *node_name,
                const char *node_state,
                const char *node_heartbeat_timestamp) {
        NodeConnection *node_connection = malloc0(sizeof(NodeConnection));
        node_connection->name = strdup(node_name);
        node_connection->state = strdup(node_state);
        node_connection->heartbeat_timestamp = strdup(node_heartbeat_timestamp);

        LIST_APPEND(nodes, nodes->nodes, node_connection);
}

static void nodes_update_connection(
                Nodes *nodes,
                const char *node_name,
                const char *node_state,
                const char *node_heartbeat_timestamp) {
        NodeConnection *curr = NULL;
        NodeConnection *next = NULL;
        LIST_FOREACH_SAFE(nodes, curr, next, nodes->nodes) {
                if (streq(curr->name, node_name)) {
                        free(curr->state);
                        curr->state = strdup(node_state);
                        curr->heartbeat_timestamp = strdup(node_heartbeat_timestamp);
                }
        }
}

static void nodes_unref(Nodes *nodes) {
        if (nodes == NULL) {
                return;
        }

        NodeConnection *curr = NULL;
        NodeConnection *next = NULL;
        LIST_FOREACH_SAFE(nodes, curr, next, nodes->nodes) {
                free(curr->name);
                free(curr->state);
                free(curr->heartbeat_timestamp);
        }

        free(nodes);
        nodes = NULL;
}

DEFINE_CLEANUP_FUNC(Nodes, nodes_unref)
#define _cleanup_nodes_ _cleanup_(nodes_unrefp)

static void print_nodes(Nodes *nodes) {
        /* clear screen */
        printf("\033[2J");
        printf("\033[%d;%dH", 0, 0);

        /* print monitor header */
        printf("%-30.30s|%10s|%20s|\n", "NODE", "STATE", "LAST HEARTBEAT");
        printf("===============================================================\n");

        NodeConnection *curr = NULL;
        NodeConnection *next = NULL;
        LIST_FOREACH_SAFE(nodes, curr, next, nodes->nodes) {
                printf("%-30.30s|%10s|%20s|\n", curr->name, curr->state, curr->heartbeat_timestamp);
        }
}

static int on_node_connection_state_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Nodes *nodes = userdata;

        const char *node_name = NULL, *con_state = NULL, *heartbeat_timestamp = NULL;

        int r = 0;
        r = sd_bus_message_read(m, "sss", &node_name, &con_state, &heartbeat_timestamp);
        if (r < 0) {
                fprintf(stderr, "Can't parse node connection state changed signal: %s\n", strerror(-r));
                return 0;
        }

        nodes_update_connection(nodes, node_name, con_state, heartbeat_timestamp);
        print_nodes(nodes);

        return 0;
}

int method_monitor_node_connection_state(sd_bus *api_bus) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        api_bus,
                        HIRTE_INTERFACE_BASE_NAME,
                        HIRTE_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        "ListNodes",
                        &error,
                        &reply,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to list nodes: %s\n", error.message);
                return r;
        }

        _cleanup_nodes_ Nodes *nodes = nodes_new();

        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open reply array: %s\n", strerror(-r));
                return r;
        }
        while (sd_bus_message_at_end(reply, false) == 0) {
                const char *name = NULL;
                UNUSED const char *path = NULL;
                const char *state = NULL;
                const char *timestamp = NULL;

                r = sd_bus_message_read(reply, "(soss)", &name, &path, &state, &timestamp);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }

                nodes_add_connection(nodes, name, state, timestamp);
        }
        print_nodes(nodes);

        r = sd_bus_match_signal(
                        api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
                        HIRTE_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        "NodeConnectionStateChanged",
                        on_node_connection_state_changed,
                        nodes);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for NodeConnectionStateChanged: %s\n", strerror(-r));
                return r;
        }

        return start_event_loop(api_bus);
}
