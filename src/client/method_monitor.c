/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "libbluechi/common/common.h"
#include "libbluechi/common/list.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/service/shutdown.h"

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
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
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
                        api_bus,
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
                        api_bus,
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
                        api_bus,
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
                        api_bus, &m, BC_INTERFACE_BASE_NAME, monitor_path, MONITOR_INTERFACE, "SubscribeList");
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

        r = sd_bus_call(api_bus, m, BC_DEFAULT_DBUS_TIMEOUT, &error, &reply);
        if (r < 0) {
                fprintf(stderr, "Failed to subscribe to monitor: %s\n", error.message);
                return r;
        }

        return start_event_loop(api_bus);
}


/***************************************************************
 ******** Monitor: Connection state changes of agents **********
 ***************************************************************/

typedef struct Node Node;
typedef struct Nodes Nodes;
typedef struct NodeConnection NodeConnection;

struct NodeConnection {
        char *name;
        char *node_path;
        char *state;
        uint64_t last_seen;
};

typedef struct Node {
        sd_bus *api_bus;
        NodeConnection *connection;
        LIST_FIELDS(Node, nodes);
        Nodes *nodes;
} Node;

typedef struct Nodes {
        LIST_HEAD(Node, nodes);
} Nodes;

static Node *
                node_new(sd_bus *api_bus,
                         Nodes *head,
                         const char *node_name,
                         const char *node_path,
                         const char *node_state,
                         uint64_t last_seen_timestamp) {
        Node *node = malloc0(sizeof(Node));
        node->connection = malloc0(sizeof(NodeConnection));
        node->connection->name = strdup(node_name);
        node->connection->node_path = strdup(node_path);
        node->connection->state = strdup(node_state);
        node->connection->last_seen = last_seen_timestamp;
        node->api_bus = api_bus;
        node->nodes = head;
        LIST_APPEND(nodes, head->nodes, node);
        return node;
}

static Nodes *nodes_new() {
        Nodes *nodes = malloc0(sizeof(Nodes));
        LIST_HEAD_INIT(nodes->nodes);
        return nodes;
}

static char *node_connection_fmt_last_seen(NodeConnection *con) {
        if (streq(con->state, "online")) {
                return strdup("now");
        } else if (streq(con->state, "offline") && con->last_seen == 0) {
                return strdup("never");
        }

        struct timespec t;
        t.tv_sec = (time_t) con->last_seen;
        t.tv_nsec = 0;
        return get_formatted_log_timestamp_for_timespec(t, false);
}

static void node_unref(Node *node) {
        if (node == NULL) {
                return;
        }

        if (node->connection != NULL) {
                free(node->connection->name);
                free(node->connection->node_path);
                free(node->connection->state);
                free(node->connection);
                node->connection = NULL;
        }

        if (node->nodes != NULL) {
                node->nodes = NULL;
        }

        free(node);
        node = NULL;
}


static void nodes_unref(Nodes *nodes) {
        if (nodes == NULL) {
                return;
        }

        Node *curr = NULL;
        Node *next = NULL;
        LIST_FOREACH_SAFE(nodes, curr, next, nodes->nodes) {
                node_unref(curr);
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
        printf("%-30.30s| %-10.10s| %-28.28s\n", "NODE", "STATE", "LAST SEEN");
        printf("=========================================================================\n");

        Node *curr = NULL;
        Node *next = NULL;
        LIST_FOREACH_SAFE(nodes, curr, next, nodes->nodes) {
                _cleanup_free_ char *last_seen = node_connection_fmt_last_seen(curr->connection);
                printf("%-30.30s| %-10.10s| %-28.28s\n",
                       curr->connection->name,
                       curr->connection->state,
                       last_seen);
        }
}


static int fetch_last_seen_timestamp_property(
                sd_bus *api_bus, const char *node_path, uint64_t *ret_last_seen_timestamp) {
        _cleanup_sd_bus_error_ sd_bus_error prop_error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *prop_reply = NULL;

        int r = sd_bus_get_property(
                        api_bus,
                        BC_INTERFACE_BASE_NAME,
                        node_path,
                        NODE_INTERFACE,
                        "LastSeenTimestamp",
                        &prop_error,
                        &prop_reply,
                        "t");
        if (r < 0) {
                return r;
        }

        uint64_t last_seen_timestamp = 0;
        r = sd_bus_message_read(prop_reply, "t", &last_seen_timestamp);
        if (r < 0) {
                return r;
        }
        *ret_last_seen_timestamp = last_seen_timestamp;

        return 0;
}

static int parse_status_from_changed_properties(sd_bus_message *m, char **ret_connection_status) {
        if (ret_connection_status == NULL) {
                fprintf(stderr, "NULL pointer to connection status not allowed");
                return -EINVAL;
        }

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

                /* only process property changes for the node status */
                if (!streq(key, "Status")) {
                        break;
                }

                /* node status is always of type string */
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
                if (r < 0) {
                        fprintf(stderr, "Failed to enter content of variant type: %s", strerror(-r));
                        return r;
                }
                char *status = NULL;
                r = sd_bus_message_read(m, "s", &status);
                if (r < 0) {
                        fprintf(stderr, "Failed to read value of changed property: %s\n", strerror(-r));
                        return r;
                }
                *ret_connection_status = strdup(status);

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit container: %s\n", strerror(-r));
                        return r;
                }
        }

        return sd_bus_message_exit_container(m);
}

static int on_node_connection_state_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;

        (void) sd_bus_message_skip(m, "s");

        _cleanup_free_ char *con_state = NULL;
        int r = parse_status_from_changed_properties(m, &con_state);
        if (r < 0) {
                return r;
        }
        if (con_state == NULL) {
                fprintf(stderr, "Received connection status change signal with missing 'Status' key.");
                return 0;
        }

        r = fetch_last_seen_timestamp_property(
                        node->api_bus, node->connection->node_path, &node->connection->last_seen);
        if (r < 0) {
                fprintf(stderr,
                        "Failed to get last seen property of node %s: %s\n",
                        node->connection->name,
                        strerror(-r));
                return r;
        }

        free(node->connection->state);
        node->connection->state = strdup(con_state);

        print_nodes(node->nodes);

        return 0;
}

int method_monitor_node_connection_state(sd_bus *api_bus) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
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

        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(sos)");
        if (r < 0) {
                fprintf(stderr, "Failed to open reply array: %s\n", strerror(-r));
                return r;
        }
        while (sd_bus_message_at_end(reply, false) == 0) {
                const char *name = NULL;
                const char *path = NULL;
                const char *state = NULL;
                uint64_t last_seen_timestamp = 0;

                r = sd_bus_message_read(reply, "(sos)", &name, &path, &state);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }

                if (streq(state, "offline")) {
                        r = fetch_last_seen_timestamp_property(api_bus, path, &last_seen_timestamp);
                        if (r < 0) {
                                fprintf(stderr,
                                        "Failed to get last seen property of node %s: %s\n",
                                        name,
                                        strerror(-r));
                                return r;
                        }
                }

                Node *node = node_new(api_bus, nodes, name, path, state, last_seen_timestamp);
                r = sd_bus_match_signal(
                                api_bus,
                                NULL,
                                BC_INTERFACE_BASE_NAME,
                                path,
                                "org.freedesktop.DBus.Properties",
                                "PropertiesChanged",
                                on_node_connection_state_changed,
                                node);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to create callback for NodeConnectionStateChanged: %s\n",
                                strerror(-r));
                        return r;
                }
        }

        print_nodes(nodes);

        return start_event_loop(api_bus);
}
