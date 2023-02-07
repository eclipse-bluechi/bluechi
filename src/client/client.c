#include "libhirte/bus/utils.h"
#include "libhirte/log/log.h"

#include "client.h"

int list_units_on_all(UNUSED Client *client) {
        hirte_log_info("Listing units on all\n");
        return 0;
}

int list_units_on(const char *name, Client *client) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;

        int r = 0;

        hirte_log_infof("Listing units on %s\n", name);

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, name, &client->object_path);
        if (r < 0) {
                hirte_log_errorf("Failed to assemble object path: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call_method(
                        client->user_bus,
                        HIRTE_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "ListUnits",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                hirte_log_errorf("Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, UNIT_INFO_STRUCT_TYPESTRING);
        if (r < 0) {
                hirte_log_errorf("Failed to read sd-bus message: %s\n", strerror(-r));
                return r;
        }

        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        for (;;) {
                _cleanup_unit_ UnitInfo *info = new_unit();

                r = bus_parse_unit_info(message, info);
                if (r < 0) {
                        hirte_log_errorf("Failed to parse unit info: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                LIST_APPEND(units, unit_list->units, unit_ref(info));
        }

        print_unit_list(&client->printer, unit_list);

        return 0;
}

Client *new_client(char *op, int opargc, char **opargv) {
        Client *client = malloc0(sizeof(Client));
        if (client == NULL) {
                return NULL;
        }

        client->ref_count = 1;
        client->op = op;
        client->opargc = opargc;
        client->opargv = opargv;
        client->printer = get_simple_printer();

        return client;
}

Client *client_ref(Client *client) {
        client->ref_count++;
        return client;
}

void client_unref(Client *client) {
        client->ref_count--;
        if (client->ref_count != 0) {
                return;
        }

        if (client->user_bus != NULL) {
                sd_bus_unrefp(&client->user_bus);
        }
        if (client->object_path != NULL) {
                free(client->object_path);
        }

        free(client);
}

UnitList *new_unit_list() {
        _cleanup_unit_list_ UnitList *unit_list = malloc0(sizeof(UnitList));

        if (unit_list == NULL) {
                return NULL;
        }

        unit_list->ref_count = 1;
        LIST_HEAD_INIT(unit_list->units);

        return steal_pointer(&unit_list);
}

UnitList *unit_list_ref(UnitList *unit_list) {
        unit_list->ref_count++;
        return unit_list;
}

void unit_list_unref(UnitList *unit_list) {
        unit_list->ref_count--;
        if (unit_list->ref_count != 0) {
                return;
        }

        UnitInfo *unit = NULL, *next_unit = NULL;
        LIST_FOREACH_SAFE(units, unit, next_unit, unit_list->units) {
                unit_unref(unit);
        }
        free(unit_list);
}

NodesUnitList *new_nodes_unit_list() {
        _cleanup_nodes_unit_list_ NodesUnitList *nodes_unit_list = malloc0(sizeof(NodesUnitList));

        if (nodes_unit_list == NULL) {
                return NULL;
        }

        nodes_unit_list->ref_count = 1;
        LIST_HEAD_INIT(nodes_unit_list->nodes_units);

        return steal_pointer(&nodes_unit_list);
}

NodesUnitList *nodes_unit_list_ref(NodesUnitList *nodes_unit_list) {
        nodes_unit_list->ref_count++;
        return nodes_unit_list;
}

void nodes_unit_list_unref(NodesUnitList *nodes_unit_list) {
        nodes_unit_list->ref_count--;
        if (nodes_unit_list->ref_count != 0) {
                return;
        }

        UnitList *node_unit_list = NULL, *next_node_unit_list = NULL;
        LIST_FOREACH_SAFE(node_units, node_unit_list, next_node_unit_list, nodes_unit_list->nodes_units) {
                unit_list_unref(node_unit_list);
        }
        free(nodes_unit_list);
}

int client_call_manager(Client *client) {
        int r = 0;

        r = sd_bus_open_user(&(client->user_bus));
        if (r < 0) {
                hirte_log_errorf("Failed to connect to system bus: %s\n", strerror(-r));
                return r;
        }

        if (streq(client->op, "list-units")) {
                if (client->opargc == 0) {
                        r = list_units_on_all(client);
                } else {
                        r = list_units_on(client->opargv[0], client);
                }
        }

        return r;
}
