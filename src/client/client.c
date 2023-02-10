#include "libhirte/bus/utils.h"

#include "client.h"

int fetch_unit_list(
                Client *client,
                char *interface,
                char *typestring,
                int (*parse_unit_info)(sd_bus_message *, UnitInfo *),
                UnitList *unit_list) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;

        int r = 0;

        r = sd_bus_call_method(
                        client->api_bus,
                        HIRTE_INTERFACE_BASE_NAME,
                        client->object_path,
                        interface,
                        "ListUnits",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, typestring);
        if (r < 0) {
                fprintf(stderr, "Failed to read sd-bus message: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                _cleanup_unit_ UnitInfo *info = new_unit();

                r = (*parse_unit_info)(message, info);
                if (r < 0) {
                        fprintf(stderr, "Failed to parse unit info: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                LIST_APPEND(units, unit_list->units, unit_ref(info));
        }

        return r;
}

int list_units_on_all(Client *client) {
        int r = 0;
        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        r = asprintf(&client->object_path, "%s", HIRTE_OBJECT_PATH);
        if (r < 0) {
                return r;
        }

        r = fetch_unit_list(
                        client,
                        MANAGER_INTERFACE,
                        NODE_AND_UNIT_INFO_STRUCT_TYPESTRING,
                        &bus_parse_unit_on_node_info,
                        unit_list);
        if (r < 0) {
                return r;
        }

        print_nodes_unit_list(&client->printer, unit_list);

        return 0;
}

int list_units_on(const char *name, Client *client) {
        int r = 0;
        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = fetch_unit_list(
                        client, NODE_INTERFACE, UNIT_INFO_STRUCT_TYPESTRING, &bus_parse_unit_info, unit_list);
        if (r < 0) {
                return r;
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

        if (client->api_bus != NULL) {
                sd_bus_unrefp(&client->api_bus);
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

int client_call_manager(Client *client) {
        int r = 0;

#ifdef USE_USER_API_BUS
        r = sd_bus_open_user(&(client->api_bus));
#else
        r = sd_bus_open_system(&(client->api_bus));
#endif
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
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
