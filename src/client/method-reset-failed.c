/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-reset-failed.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/opt.h"

typedef struct NAME_LIST NAME_LIST;

typedef struct NAME_LIST {
        char *name;
        NAME_LIST *next;
} NAME_LIST;

void free_list(NAME_LIST *head) {
        NAME_LIST *tmp_head = NULL;
        do {
                tmp_head = head;
                head = head->next;
                free(tmp_head);

        } while (head->next);
        free(head);
}

int get_all_nodes_list(Client *client, NAME_LIST *head) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "ListNodes",
                        &error,
                        &result,
                        "",
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(result, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open result array: %s\n", strerror(-r));
                return r;
        }

        NAME_LIST *tmp_head = head;
        while (sd_bus_message_at_end(result, false) == 0) {
                const char *name = NULL;

                r = sd_bus_message_read(result, "(soss)", &name, NULL, NULL, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }
                NAME_LIST *next = malloc(sizeof(NAME_LIST));
                if (next == NULL) {
                        free_list(head);
                        fprintf(stderr, "Out of memory\n");
                        return -ENOMEM;
                }
                next->name = strdup(name);
                tmp_head->next = next;
                tmp_head = next;
        }

        return r;
}

int get_all_units_list(Client *client, NAME_LIST *head, char *node_name) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "ListUnits",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr, "Couldn't get ListUnits of '%s': %s\n", node_name, error.message);
                return r;
        }

        r = sd_bus_message_enter_container(result, SD_BUS_TYPE_ARRAY, "(ssssssouso)");
        if (r < 0) {
                fprintf(stderr, "Failed to open result array: %s\n", strerror(-r));
                return r;
        }

        NAME_LIST *tmp_head = head;
        while (sd_bus_message_at_end(result, false) == 0) {
                const char *name = NULL;

                r = sd_bus_message_read(
                                result, "(ssssssouso)", &name, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }
                NAME_LIST *next = malloc(sizeof(NAME_LIST));
                if (next == NULL) {
                        free(head);
                        fprintf(stderr, "Out of memory\n");
                        return -ENOMEM;
                }
                next->name = strdup(name);
                tmp_head->next = next;
                tmp_head = next;
        }

        return r;
}

static int method_reset_failed_on_node(Client *client, char *node_name) {

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        int r = 0;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "ResetFailed",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr,
                        "Couldn't reset failed state of all units on node '%s': %s\n",
                        node_name,
                        error.message);
                return r;
        }

        return r;
}

static int method_reset_failed_on_unit(Client *client, char *node_name, char *unit_name) {

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        int r = 0;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        "ResetFailedUnit",
                        &error,
                        &result,
                        "s",
                        unit_name);

        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        return r;
}

int method_reset_failed_on_node_with_units(Client *client, Command *commands, char *node_name) {
        int units_count = 1;
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }
        if (commands->opargv[units_count] == NULL) {
                return method_reset_failed_on_node(client, node_name);
        } else {
                while (commands->opargv[units_count] != NULL) {
                        bool is_unit_name_glob = is_glob((const char *) commands->opargv[units_count]);
                        NAME_LIST *unit_name_head = malloc(sizeof(NAME_LIST));
                        if (unit_name_head == NULL) {
                                fprintf(stderr, "Out of memory\n");
                                return -ENOMEM;
                        }
                        NAME_LIST *tmp_head = NULL;
                        char *unit_name = commands->opargv[units_count];
                        if (is_unit_name_glob) {
                                r = get_all_units_list(client, unit_name_head, node_name);
                                if (r < 0) {
                                        return r;
                                }
                                tmp_head = unit_name_head->next;
                                while (tmp_head) {
                                        if (match_glob(tmp_head->name, unit_name)) {
                                                r = method_reset_failed_on_unit(
                                                                client, node_name, tmp_head->name);
                                                if (r < 0) {
                                                        return r;
                                                }
                                        }
                                        tmp_head = tmp_head->next;
                                }
                                free_list(unit_name_head);
                        } else {
                                r = method_reset_failed_on_unit(client, node_name, unit_name);
                                if (r < 0) {
                                        return r;
                                }
                        }
                        units_count++;
                }
        }
        return r;
}

static int method_reset_failed_with_params(UNUSED Client *client, UNUSED Command *commands) {
        char *node_name = commands->opargv[0];
        int r = 0;

        NAME_LIST *node_name_head = malloc(sizeof(NAME_LIST));
        if (node_name_head == NULL) {
                fprintf(stderr, "Out of memory\n");
                return -ENOMEM;
        }

        bool is_node_name_glob = is_glob((const char *) node_name);
        if (is_node_name_glob) {
                r = get_all_nodes_list(client, node_name_head);
                if (r < 0) {
                        return r;
                }
                if (node_name_head->next == NULL) {
                        fprintf(stderr, "No Node name found\n");
                        return r;
                }
                NAME_LIST *tmp_head = node_name_head->next;
                do {
                        int skip = 1;
                        if (!match_glob(tmp_head->name, node_name)) {
                                skip = 0;
                        }
                        if (skip) {
                                r = method_reset_failed_on_node_with_units(client, commands, tmp_head->name);
                                if (r < 0) {
                                        return r;
                                }
                        }
                        tmp_head = tmp_head->next;

                } while (tmp_head->next);


                free_list(node_name_head);
        } else {
                r = method_reset_failed_on_node_with_units(client, commands, node_name);
                if (r < 0) {
                        return r;
                }
        }


        return r;
}

static int method_reset_failed_on_all_nodes(Client *client) {

        int r = 0;

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "ListNodes",
                        &error,
                        &result,
                        "",
                        NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_enter_container(result, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open result array: %s\n", strerror(-r));
                return r;
        }
        while (sd_bus_message_at_end(result, false) == 0) {
                const char *name = NULL;

                r = sd_bus_message_read(result, "(soss)", &name, NULL, NULL, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to read node information: %s\n", strerror(-r));
                        return r;
                }

                _cleanup_free_ char *node_name = strdup(name);
                r = method_reset_failed_on_node(client, node_name);
        }

        return r;
}

int method_reset_failed(Command *command, void *userdata) {
        if (command->opargv[0] != NULL) {
                return method_reset_failed_with_params(userdata, command);
        }
        return method_reset_failed_on_all_nodes(userdata);
}

void usage_method_reset_failed() {
        usage_print_header();
        usage_print_description("Reset the failed state of units");
        usage_print_usage("bluechictl reset-failed [nodename] [unit1, unit2, ...]");
        printf("  If [nodename] and unit(s) are not given, the failed state is reset for all units on all nodes.\n");
        printf("  If no unit(s) are given, the failed state is reset for all units on the given node.\n");
}
