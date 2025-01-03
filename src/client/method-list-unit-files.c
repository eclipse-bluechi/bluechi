/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "client.h"
#include "method-list-unit-files.h"
#include "usage.h"

#include "libbluechi/common/math-util.h"
#include "libbluechi/common/opt.h"
#include "libbluechi/common/string-util.h"

typedef void (*print_unit_file_list_fn)(UnitFileList *unit_file_list, const char *glob_filter);

struct UnitFileList {
        int ref_count;

        LIST_FIELDS(UnitFileList, node_units);

        LIST_HEAD(UnitFileInfo, unit_files);
};

UnitFileList *new_unit_file_list();
void unit_file_list_unref(UnitFileList *unit_file_list);

DEFINE_CLEANUP_FUNC(UnitFileList, unit_file_list_unref)
#define _cleanup_unit_file_list_ _cleanup_(unit_file_list_unrefp)


static int parse_unit_file_list(sd_bus_message *message, const char *node_name, UnitFileList *unit_file_list) {

        int r = 0;
        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, UNIT_FILE_INFO_STRUCT_TYPESTRING);
        if (r < 0) {
                fprintf(stderr, "Failed to enter sd-bus message container: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                _cleanup_unit_file_ UnitFileInfo *info = new_unit_file();

                r = bus_parse_unit_file_info(message, info);
                if (r < 0) {
                        fprintf(stderr, "Failed to parse unit info: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                info->node = strdup(node_name);
                if (info->node == NULL) {
                        return -ENOMEM;
                }

                LIST_APPEND(unit_files, unit_file_list->unit_files, unit_file_ref(info));
        }

        r = sd_bus_message_exit_container(message);
        if (r < 0) {
                fprintf(stderr, "Failed to exit sd-bus message container: %s\n", strerror(-r));
                return r;
        }

        return r;
}

static int method_list_unit_files_on_all(sd_bus *api_bus, print_unit_file_list_fn print, const char *glob_filter) {
        int r = 0;
        _cleanup_unit_file_list_ UnitFileList *unit_file_list = new_unit_file_list();

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        r = sd_bus_call_method(
                        api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "ListUnitFiles",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, NODE_AND_UNIT_FILE_INFO_DICT_TYPESTRING);
        if (r < 0) {
                fprintf(stderr, "Failed to read sd-bus message: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                r = sd_bus_message_enter_container(
                                message, SD_BUS_TYPE_DICT_ENTRY, NODE_AND_UNIT_FILE_INFO_TYPESTRING);
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
                parse_unit_file_list(message, node_name, unit_file_list);

                r = sd_bus_message_exit_container(message);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit sd-bus message dictionary: %s\n", strerror(-r));
                        return r;
                }
        }

        print(unit_file_list, glob_filter);

        return 0;
}

static int method_list_unit_files_on(
                sd_bus *api_bus, const char *node_name, print_unit_file_list_fn print, const char *glob_filter) {
        int r = 0;
        _cleanup_unit_file_list_ UnitFileList *unit_file_list = new_unit_file_list();

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
                        "ListUnitFiles",
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = parse_unit_file_list(message, node_name, unit_file_list);
        if (r < 0) {
                return r;
        }

        print(unit_file_list, glob_filter);

        return 0;
}


/****************************
 ******** UnitFileList ******
 ****************************/

UnitFileList *new_unit_file_list() {
        _cleanup_unit_file_list_ UnitFileList *unit_file_list = malloc0(sizeof(UnitFileList));

        if (unit_file_list == NULL) {
                return NULL;
        }

        unit_file_list->ref_count = 1;
        LIST_HEAD_INIT(unit_file_list->unit_files);

        return steal_pointer(&unit_file_list);
}

void unit_file_list_unref(UnitFileList *unit_file_list) {
        unit_file_list->ref_count--;
        if (unit_file_list->ref_count != 0) {
                return;
        }

        UnitFileInfo *unit_file = NULL, *next_unit_file = NULL;
        LIST_FOREACH_SAFE(unit_files, unit_file, next_unit_file, unit_file_list->unit_files) {
                unit_file_unref(unit_file);
        }
        free(unit_file_list);
}

void print_unit_file_list_simple(UnitFileList *unit_file_list, const char *glob_filter) {
        UnitFileInfo *unit_file = NULL;
        const unsigned int FMT_STR_MAX_LEN = 255;
        char fmt_str[FMT_STR_MAX_LEN];
        const char node_title[] = "NODE";
        const char path_title[] = "PATH";
        const char enablment_title[] = "IS-ENABLED";
        const char col_sep[] = " | ";

        unsigned long max_node_len = strlen(node_title);
        unsigned long max_path_len = strlen(path_title);
        unsigned long max_enablement_len = strlen(enablment_title);
        unsigned long sep_len = strlen(col_sep);

        LIST_FOREACH(unit_files, unit_file, unit_file_list->unit_files) {
                max_node_len = umaxl(max_node_len, strlen(unit_file->node));
                max_path_len = umaxl(max_path_len, strlen(unit_file->unit_path));
                max_enablement_len = umaxl(max_enablement_len, strlen(unit_file->enablement_status));
        }

        unsigned long max_line_len = max_node_len + sep_len + max_path_len + sep_len + max_enablement_len;
        char sep_line[max_line_len + 1];

        snprintf(fmt_str,
                 FMT_STR_MAX_LEN,
                 "%%-%lus%s%%-%lus%s%%-%lus\n",
                 max_node_len,
                 col_sep,
                 max_path_len,
                 col_sep,
                 max_enablement_len);

        printf(fmt_str, "NODE", "PATH", "IS-ENABLED");
        memset(&sep_line, '=', sizeof(char) * max_line_len);
        sep_line[max_line_len] = '\0';
        printf("%s\n", sep_line);

        LIST_FOREACH(unit_files, unit_file, unit_file_list->unit_files) {
                if (glob_filter == NULL || match_glob(unit_file->unit_path, glob_filter)) {
                        printf(fmt_str, unit_file->node, unit_file->unit_path, unit_file->enablement_status);
                }
        }
}

int method_list_unit_files(Command *command, void *userdata) {
        Client *client = (Client *) userdata;
        char *filter_glob = command_get_option(command, ARG_FILTER_SHORT);
        if (command->opargc == 0) {
                return method_list_unit_files_on_all(client->api_bus, print_unit_file_list_simple, filter_glob);
        }
        return method_list_unit_files_on(
                        client->api_bus, command->opargv[0], print_unit_file_list_simple, filter_glob);
}

void usage_method_list_unit_files() {
        usage_print_header();
        usage_print_description("Get a list of installed systemd unit files");
        usage_print_usage("bluechictl list-unit-files [nodename] [options]");
        printf("  If [nodename] is not given, the systemd unit files of all nodes are queried.\n");
        printf("\n");
        printf("Available options:\n");
        printf("  --%s \t shows this help message\n", ARG_HELP);
        printf("  --%s \t filter the queried systemd unit files by name using a glob\n", ARG_FILTER);
}
