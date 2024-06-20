/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-status.h"
#include "client.h"

#include "libbluechi/bus/utils.h"
#include "libbluechi/common/common.h"
#include "libbluechi/common/opt.h"
#include "libbluechi/common/time-util.h"

typedef struct unit_info_t {
        const char *id;
        const char *load_state;
        const char *active_state;
        const char *freezer_state;
        const char *sub_state;
        const char *unit_file_state;
} unit_info_t;

struct bus_properties_map {
        const char *member;
        size_t offset;
};

typedef struct MethodStatusChange MethodStatusChange;

struct MethodStatusChange {
        Client *client;
        char **units;
        char *node_name;
        size_t units_count;
        size_t max_len;
};

void clear_screen() {
        printf("\033[2J");
        printf("\033[%d;%dH", 0, 0);
}

static const struct bus_properties_map property_map[] = {
        { "Id", offsetof(unit_info_t, id) },
        { "LoadState", offsetof(unit_info_t, load_state) },
        { "ActiveState", offsetof(unit_info_t, active_state) },
        { "FreezerState", offsetof(unit_info_t, freezer_state) },
        { "SubState", offsetof(unit_info_t, sub_state) },
        { "UnitFileState", offsetof(unit_info_t, unit_file_state) },
        { NULL, 0 },
};

static int get_property_map(sd_bus_message *m, const struct bus_properties_map **prop) {
        int r = 0;
        const char *member = NULL;
        int i = 0;

        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &member);
        if (r < 0) {
                fprintf(stderr, "Failed to read the name of the property: %s\n", strerror(-r));
                return r;
        }

        for (i = 0; property_map[i].member; i++) {
                if (streq(property_map[i].member, member)) {
                        *prop = &property_map[i];
                        break;
                }
        }

        return 0;
}

static int read_property_value(sd_bus_message *m, const struct bus_properties_map *prop, unit_info_t *unit_info) {
        int r = 0;
        char type = 0;
        const char *contents = NULL;
        const char *s = NULL;
        const char **v = (const char **) ((uint8_t *) unit_info + prop->offset);

        r = sd_bus_message_peek_type(m, NULL, &contents);
        if (r < 0) {
                fprintf(stderr, "Failed to peek into the type of the property: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, contents);
        if (r < 0) {
                fprintf(stderr, "Failed to enter into the container of the property: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_message_peek_type(m, &type, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to get the type of the property: %s\n", strerror(-r));
                return r;
        }

        if (type != SD_BUS_TYPE_STRING) {
                fprintf(stderr, "Currently only string types are expected\n");
                return -EINVAL;
        }

        r = sd_bus_message_read_basic(m, type, &s);
        if (r < 0) {
                fprintf(stderr, "Failed to get the value of the property: %s\n", strerror(-r));
                return r;
        }

        if (isempty(s)) {
                s = NULL;
        }

        *v = s;

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                fprintf(stderr, "Failed to exit from the container of the property: %s\n", strerror(-r));
        }

        return r;
}

static int process_dict_entry(sd_bus_message *m, unit_info_t *unit_info) {
        int r = 0;
        const struct bus_properties_map *prop = NULL;

        r = get_property_map(m, &prop);
        if (r < 0) {
                return r;
        }

        if (prop) {
                r = read_property_value(m, prop, unit_info);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to get the value for member %s - %s\n",
                                prop->member,
                                strerror(-r));
                }
        } else {
                r = sd_bus_message_skip(m, "v");
                if (r < 0) {
                        fprintf(stderr, "Failed to skip the property container: %s\n", strerror(-r));
                }
        }

        return r;
}

static int parse_unit_status_response_from_message(sd_bus_message *m, unit_info_t *unit_info) {
        int r = 0;

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                fprintf(stderr, "Failed to open the strings array container: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                if (r < 0) {
                        fprintf(stderr, "Failed to enter the property container: %s\n", strerror(-r));
                        return r;
                }

                if (r == 0) {
                        break;
                }

                r = process_dict_entry(m, unit_info);
                if (r < 0) {
                        break;
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit the property container: %s\n", strerror(-r));
                        break;
                }
        }

        return r;
}

#define PRINT_TAB_SIZE 8
static void print_info_header(size_t name_col_width) {
        size_t i = 0;

        fprintf(stdout, "UNIT");
        for (i = PRINT_TAB_SIZE + name_col_width; i > PRINT_TAB_SIZE; i -= PRINT_TAB_SIZE) {
                fprintf(stdout, "\t");
        }

        fprintf(stdout, "| LOADED\t| ACTIVE\t| SUBSTATE\t| FREEZERSTATE\t| ENABLED\t|\n");
        for (i = PRINT_TAB_SIZE + name_col_width; i > PRINT_TAB_SIZE; i -= PRINT_TAB_SIZE) {
                fprintf(stdout, "--------");
        }
        fprintf(stdout, "----------------");
        fprintf(stdout, "----------------");
        fprintf(stdout, "----------------");
        fprintf(stdout, "----------------");
        fprintf(stdout, "----------------\n");
}

#define PRINT_AND_ALIGN(x)                                      \
        do {                                                    \
                fprintf(stdout, "| %s\t", unit_info->x);        \
                if (unit_info->x && strlen(unit_info->x) < 6) { \
                        fprintf(stdout, "\t");                  \
                }                                               \
        } while (0)


static void print_unit_info(unit_info_t *unit_info, size_t name_col_width) {
        size_t i = 0;

        if (unit_info->load_state && streq(unit_info->load_state, "not-found")) {
                fprintf(stderr, "Unit %s could not be found.\n", unit_info->id);
                return;
        }

        fprintf(stdout, "%s", unit_info->id ? unit_info->id : "");
        name_col_width -= unit_info->id ? strlen(unit_info->id) : 0;
        name_col_width += PRINT_TAB_SIZE;
        i = name_col_width;
        while (i >= PRINT_TAB_SIZE) {
                fprintf(stdout, "\t");
                i -= PRINT_TAB_SIZE;
        }
        while (i > PRINT_TAB_SIZE) {
                fprintf(stdout, " ");
                i--;
        }


        PRINT_AND_ALIGN(load_state);
        PRINT_AND_ALIGN(active_state);
        PRINT_AND_ALIGN(sub_state);
        PRINT_AND_ALIGN(freezer_state);
        PRINT_AND_ALIGN(unit_file_state);
        fflush(stdout);

        fprintf(stdout, "|\n");
}

static size_t get_max_name_len(char **units, size_t units_count) {
        size_t i = 0;
        size_t max_unit_name_len = 0;

        for (i = 0; i < units_count; i++) {
                size_t unit_name_len = strlen(units[i]);
                max_unit_name_len = max_unit_name_len > unit_name_len ? max_unit_name_len : unit_name_len;
        }

        return max_unit_name_len;
}

static int get_status_unit_on(Client *client, char *node_name, char *unit_name, size_t name_col_width) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;
        unit_info_t unit_info = { 0 };

        r = client_create_message_new_method_call(client, node_name, "GetUnitProperties", &outgoing_message);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_message_append(outgoing_message, "ss", unit_name, "");
        if (r < 0) {
                fprintf(stderr, "Failed to append runtime to the message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, outgoing_message, BC_DEFAULT_DBUS_TIMEOUT, &error, &result);
        if (r < 0) {
                fprintf(stderr, "Failed to issue call: %s\n", error.message);
                return r;
        }

        r = parse_unit_status_response_from_message(result, &unit_info);
        if (r < 0) {
                fprintf(stderr, "Failed to parse the response strings array: %s\n", error.message);
                return r;
        }

        print_unit_info(&unit_info, name_col_width);

        return 0;
}


static int on_unit_status_changed(UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        MethodStatusChange *s = (MethodStatusChange *) userdata;

        char **units = s->units;
        char *node_name = s->node_name;
        Client *client = s->client;
        size_t units_count = s->units_count;
        size_t max_len = s->max_len;

        clear_screen();
        print_info_header(max_len);
        for (unsigned i = 0; i < units_count; i++) {
                int r = get_status_unit_on(client, node_name, units[i], max_len);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to get status of unit %s on node %s - %s",
                                units[i],
                                node_name,
                                strerror(-r));
                        return r;
                }
        }

        return 1;
}


static int method_status_unit_on(
                Client *client, char *node_name, char **units, size_t units_count, bool do_watch) {
        unsigned i = 0;

        size_t max_name_len = get_max_name_len(units, units_count);
        _cleanup_free_ MethodStatusChange *s = malloc0(sizeof(MethodStatusChange));
        if (s == NULL) {
                fprintf(stderr, "Failed to malloc memory for MethodStatusChange");
                return ENOMEM;
        }
        s->client = client;
        s->max_len = max_name_len;
        s->node_name = node_name;
        s->units = units;
        s->units_count = units_count;

        print_info_header(max_name_len);
        for (i = 0; i < units_count; i++) {
                int r = get_status_unit_on(client, node_name, units[i], max_name_len);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to get status of unit %s on node %s - %s\n",
                                units[i],
                                node_name,
                                strerror(-r));
                        return r;
                }
                if (do_watch) {
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
                                fprintf(stderr,
                                        "Failed to parse create monitor response message: %s\n",
                                        strerror(-r));
                                return r;
                        }
                        r = sd_bus_match_signal(
                                        client->api_bus,
                                        NULL,
                                        BC_INTERFACE_BASE_NAME,
                                        monitor_path,
                                        MONITOR_INTERFACE,
                                        "UnitStateChanged",
                                        on_unit_status_changed,
                                        s);
                        if (r < 0) {
                                fprintf(stderr,
                                        "Failed to create callback for UnitStateChanged: %s\n",
                                        strerror(-r));
                                return r;
                        }

                        _cleanup_sd_bus_message_ sd_bus_message *m = NULL;
                        r = sd_bus_message_new_method_call(
                                        client->api_bus,
                                        &m,
                                        BC_INTERFACE_BASE_NAME,
                                        monitor_path,
                                        MONITOR_INTERFACE,
                                        "Subscribe");
                        if (r < 0) {
                                fprintf(stderr, "Failed creating subscription call: %s\n", strerror(-r));
                                return r;
                        }
                        sd_bus_message_append(m, "ss", node_name, units[i]);
                        _cleanup_sd_bus_message_ sd_bus_message *subscription_reply = NULL;
                        r = sd_bus_call(client->api_bus, m, BC_DEFAULT_DBUS_TIMEOUT, &error, &subscription_reply);
                        if (r < 0) {
                                fprintf(stderr, "Failed to subscribe to monitor: %s\n", error.message);
                                return r;
                        }
                }
        }

        return do_watch ? client_start_event_loop(client) : 0;
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
        char *ip;
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
                         const char *ip,
                         uint64_t last_seen_timestamp) {
        Node *node = malloc0(sizeof(Node));
        if (node == NULL) {
                return NULL;
        }
        node->connection = malloc0(sizeof(NodeConnection));
        if (node->connection == NULL) {
                free(node);
                node = NULL;
                return NULL;
        }
        node->connection->name = strdup(node_name);
        node->connection->node_path = strdup(node_path);
        node->connection->state = strdup(node_state);
        node->connection->ip = strdup(ip);
        node->connection->last_seen = last_seen_timestamp;
        node->api_bus = api_bus;
        node->nodes = head;
        LIST_APPEND(nodes, head->nodes, node);
        return node;
}

static Nodes *nodes_new() {
        Nodes *nodes = malloc0(sizeof(Nodes));
        if (nodes == NULL) {
                return NULL;
        }
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
        t.tv_sec = (time_t) con->last_seen / USEC_PER_SEC;
        t.tv_nsec = (long) (con->last_seen % USEC_PER_SEC) * NSEC_PER_USEC;
        return get_formatted_log_timestamp_for_timespec(t, false);
}

static void node_unref(Node *node) {
        if (node == NULL) {
                return;
        }

        if (node->connection != NULL) {
                free_and_null(node->connection->name);
                free_and_null(node->connection->node_path);
                free_and_null(node->connection->state);
                free_and_null(node->connection->ip);
                free_and_null(node->connection);
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

static void print_nodes(Nodes *nodes, bool clear_screen) {
        if (clear_screen) {
                printf("\033[2J");
                printf("\033[%d;%dH", 0, 0);
        }

        /* print monitor header */
        printf("%-30.30s| %-10.10s| %-24.24s| %-28.28s\n", "NODE", "STATE", "IP", "LAST SEEN");
        printf("==========================================================================================\n");

        Node *curr = NULL;
        Node *next = NULL;
        LIST_FOREACH_SAFE(nodes, curr, next, nodes->nodes) {
                _cleanup_free_ char *last_seen = node_connection_fmt_last_seen(curr->connection);
                printf("%-30.30s| %-10.10s| %-24.24s| %-28.28s\n",
                       curr->connection->name,
                       curr->connection->state,
                       curr->connection->ip,
                       last_seen);
                fflush(stdout);
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

        print_nodes(node->nodes, true);

        return 0;
}

static int method_print_node_status(Client *client, char *node_name, bool do_watch) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "ListNodes",
                        &error,
                        &reply,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to list nodes: %s\n", error.message);
                return r;
        }

        _cleanup_nodes_ Nodes *nodes = nodes_new();
        if (nodes == NULL) {
                fprintf(stderr, "Failed to create Node list, OOM");
                return -ENOMEM;
        }

        r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open reply array: %s\n", strerror(-r));
                return r;
        }
        while (sd_bus_message_at_end(reply, false) == 0) {
                const char *name = NULL;
                const char *path = NULL;
                const char *state = NULL;
                const char *ip = NULL;
                uint64_t last_seen_timestamp = 0;

                r = sd_bus_message_read(reply, "(soss)", &name, &path, &state, &ip);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }

                if (node_name != NULL && !streq(name, node_name)) {
                        continue;
                }

                if (streq(state, "offline")) {
                        r = fetch_last_seen_timestamp_property(client->api_bus, path, &last_seen_timestamp);
                        if (r < 0) {
                                fprintf(stderr,
                                        "Failed to get last seen property of node %s: %s\n",
                                        name,
                                        strerror(-r));
                                return r;
                        }
                }

                Node *node = node_new(client->api_bus, nodes, name, path, state, ip, last_seen_timestamp);
                if (node == NULL) {
                        fprintf(stderr, "Failed to create Node, OOM");
                        return -ENOMEM;
                }
                if (do_watch) {
                        // Create a callback for state change monitoring
                        r = sd_bus_match_signal(
                                        client->api_bus,
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
        }

        if (node_name != NULL && LIST_IS_EMPTY(nodes->nodes)) {
                fprintf(stderr, "Node %s not found\n", node_name);
                return -EINVAL;
        }

        print_nodes(nodes, do_watch);

        return do_watch ? client_start_event_loop(client) : 0;
}

int method_status(Command *command, void *userdata) {
        switch (command->opargc) {
        case 0:
                return method_print_node_status(userdata, NULL, command_flag_exists(command, ARG_WATCH_SHORT));
        case 1:
                return method_print_node_status(
                                userdata, command->opargv[0], command_flag_exists(command, ARG_WATCH_SHORT));
        default:
                return method_status_unit_on(
                                userdata,
                                command->opargv[0],
                                &command->opargv[1],
                                command->opargc - 1,
                                command_flag_exists(command, ARG_WATCH_SHORT));
        }
}
