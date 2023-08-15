/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "libbluechi/common/string-util.h"

#include "method_list_units.h"

static int
                fetch_unit_list(sd_bus *api_bus,
                                const char *node_name,
                                const char *object_path,
                                const char *interface,
                                const char *typestring,
                                int (*parse_unit_info)(sd_bus_message *, UnitInfo *),
                                UnitList *unit_list) {

        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;

        r = sd_bus_call_method(
                        api_bus, BC_INTERFACE_BASE_NAME, object_path, interface, "ListUnits", &error, &message, "");
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
                if (node_name != NULL) {
                        info->node = strdup(node_name);
                }

                LIST_APPEND(units, unit_list->units, unit_ref(info));
        }

        return r;
}

int method_list_units_on_all(sd_bus *api_bus, print_unit_list_fn print, const char *glob_filter) {
        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        int r = fetch_unit_list(
                        api_bus,
                        NULL,
                        BC_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        NODE_AND_UNIT_INFO_STRUCT_TYPESTRING,
                        &bus_parse_unit_on_node_info,
                        unit_list);
        if (r < 0) {
                return r;
        }

        print(unit_list, glob_filter);

        return 0;
}

int method_list_units_on(
                sd_bus *api_bus, const char *node_name, print_unit_list_fn print, const char *glob_filter) {
        int r = 0;
        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        _cleanup_free_ char *object_path = NULL;
        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &object_path);
        if (r < 0) {
                return r;
        }

        r = fetch_unit_list(
                        api_bus,
                        node_name,
                        object_path,
                        NODE_INTERFACE,
                        UNIT_INFO_STRUCT_TYPESTRING,
                        &bus_parse_unit_info,
                        unit_list);
        if (r < 0) {
                return r;
        }

        print(unit_list, glob_filter);

        return 0;
}


/****************************
 ******** UnitList **********
 ****************************/

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

/* Prints 100-characters-wide table */
void print_unit_list_simple(UnitList *unit_list, const char *glob_filter) {
        UnitInfo *unit = NULL;
        printf("%-20.20s|%-59.59s|%9s|%9s\n", "NODE", "ID", "ACTIVE", "SUB");
        printf("====================================================================================================\n");
        LIST_FOREACH(units, unit, unit_list->units) {
                if (glob_filter == NULL || match_glob(unit->id, glob_filter)) {
                        printf("%-20.20s|%-59.59s|%9s|%9s\n",
                               unit->node,
                               unit->id,
                               unit->active_state,
                               unit->sub_state);
                }
        }
}
