/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "method-list-units.h"
#include "usage.h"

#include "libbluechi/common/math-util.h"
#include "libbluechi/common/opt.h"
#include "libbluechi/common/string-util.h"

typedef void (*print_unit_list_fn)(UnitList *unit_list, const char *glob_filter);

struct UnitList {
        int ref_count;

        LIST_FIELDS(UnitList, node_units);

        LIST_HEAD(UnitInfo, units);
};

UnitList *new_unit_list();
void unit_list_unref(UnitList *unit_list);

DEFINE_CLEANUP_FUNC(UnitList, unit_list_unref)
#define _cleanup_unit_list_ _cleanup_(unit_list_unrefp)

static int parse_unit_list(sd_bus_message *message, const char *node_name, UnitList *unit_list) {

        int r = 0;
        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, UNIT_INFO_STRUCT_TYPESTRING);
        if (r < 0) {
                fprintf(stderr, "Failed to enter sd-bus message container: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                _cleanup_unit_ UnitInfo *info = new_unit();

                r = bus_parse_unit_info(message, info);
                if (r < 0) {
                        fprintf(stderr, "Failed to parse unit info: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                info->node = strdup(node_name);

                LIST_APPEND(units, unit_list->units, unit_ref(info));
        }

        r = sd_bus_message_exit_container(message);
        if (r < 0) {
                fprintf(stderr, "Failed to exit sd-bus message container: %s\n", strerror(-r));
                return r;
        }

        return r;
}

static int method_list_units_on_all(sd_bus *api_bus, print_unit_list_fn print, const char *glob_filter) {
        int r = 0;
        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        r = sd_bus_call_method(
                        api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "ListUnits",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, NODE_AND_UNIT_INFO_DICT_TYPESTRING);
        if (r < 0) {
                fprintf(stderr, "Failed to read sd-bus message: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                r = sd_bus_message_enter_container(
                                message, SD_BUS_TYPE_DICT_ENTRY, NODE_AND_UNIT_INFO_TYPESTRING);
                if (r < 0) {
                        fprintf(stderr, "Failed to enter sd-bus message dictionary: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }

                char *node_name = NULL;
                r = sd_bus_message_read(message, "s", &node_name);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node name: %s\n", strerror(-r));
                        return r;
                }
                parse_unit_list(message, node_name, unit_list);

                r = sd_bus_message_exit_container(message);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit sd-bus message dictionary: %s\n", strerror(-r));
                        return r;
                }
        }

        print(unit_list, glob_filter);

        return 0;
}

static int method_list_units_on(
                sd_bus *api_bus, const char *node_name, print_unit_list_fn print, const char *glob_filter) {
        int r = 0;
        _cleanup_unit_list_ UnitList *unit_list = new_unit_list();

        _cleanup_free_ char *object_path = NULL;
        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &object_path);
        if (r < 0) {
                return r;
        }

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        r = sd_bus_call_method(
                        api_bus,
                        BC_INTERFACE_BASE_NAME,
                        object_path,
                        NODE_INTERFACE,
                        "ListUnits",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = parse_unit_list(message, node_name, unit_list);
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

void print_unit_list_simple(UnitList *unit_list, const char *glob_filter) {
        UnitInfo *unit = NULL;
        const unsigned int FMT_STR_MAX_LEN = 255;
        char fmt_str[FMT_STR_MAX_LEN];
        const char node_title[] = "NODE";
        const char id_title[] = "ID";
        const char active_title[] = "ACTIVE";
        const char sub_title[] = "SUB";
        const char col_sep[] = " | ";

        unsigned long max_node_len = strlen(node_title);
        unsigned long max_id_len = strlen(id_title);
        unsigned long max_active_len = strlen(active_title);
        unsigned long max_sub_len = strlen(sub_title);
        unsigned long sep_len = strlen(col_sep);

        LIST_FOREACH(units, unit, unit_list->units) {
                max_node_len = umaxl(max_node_len, strlen(unit->node));
                max_id_len = umaxl(max_id_len, strlen(unit->id));
                max_active_len = umaxl(max_active_len, strlen(unit->active_state));
                max_sub_len = umaxl(max_sub_len, strlen(unit->sub_state));
        }

        unsigned long max_line_len = max_node_len + sep_len + max_id_len + sep_len + max_active_len +
                        sep_len + max_sub_len;
        char sep_line[max_line_len + 1];

        snprintf(fmt_str,
                 FMT_STR_MAX_LEN,
                 "%%-%lus%s%%-%lus%s%%-%lus%s%%-%lus\n",
                 max_node_len,
                 col_sep,
                 max_id_len,
                 col_sep,
                 max_active_len,
                 col_sep,
                 max_sub_len);

        printf(fmt_str, "NODE", "ID", "ACTIVE", "SUB");
        memset(&sep_line, '=', sizeof(char) * max_line_len);
        sep_line[max_line_len] = '\0';
        printf("%s\n", sep_line);

        LIST_FOREACH(units, unit, unit_list->units) {
                if (glob_filter == NULL || match_glob(unit->id, glob_filter)) {
                        printf(fmt_str, unit->node, unit->id, unit->active_state, unit->sub_state);
                }
        }
}

int method_list_units(Command *command, void *userdata) {
        Client *client = (Client *) userdata;
        char *filter_glob = command_get_option(command, ARG_FILTER_SHORT);
        if (command->opargc == 0) {
                return method_list_units_on_all(client->api_bus, print_unit_list_simple, filter_glob);
        }
        return method_list_units_on(client->api_bus, command->opargv[0], print_unit_list_simple, filter_glob);
}

void usage_method_list_units() {
        usage_print_header();
        usage_print_description("Get a list of loaded systemd units");
        usage_print_usage("bluechictl list-units [nodename] [options]");
        printf("  If [nodename] is not given, the systemd units of all nodes are queried.\n");
        printf("\n");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
        printf("  --%s \t filter the queried systemd units by name using a glob\n", ARG_FILTER);
}
