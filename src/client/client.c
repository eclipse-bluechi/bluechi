/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>

#include "libhirte/bus/utils.h"
#include "libhirte/common/time-util.h"
#include "libhirte/service/shutdown.h"

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

int method_list_units_on_all(Client *client) {
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

int method_list_units_on(const char *name, Client *client) {
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

static sd_bus_message *wait_for_job(Client *client, char *object_path) {
        int r = 0;

        assert(client->pending_job_name == NULL);
        client->pending_job_name = object_path;

        for (;;) {
                /* Process requests */
                r = sd_bus_process(client->api_bus, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
                        return NULL;
                }

                /* Did we get the result? */
                if (client->pending_job_result != NULL) {
                        return steal_pointer(&client->pending_job_result);
                }

                if (r > 0) { /* request processed, try to process another one */
                        continue;
                }

                /* Wait for the next request to process */
                r = sd_bus_wait(client->api_bus, (uint64_t) -1);
                if (r < 0) {
                        fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
                        return NULL;
                }
        }
}

static int match_job_removed_signal(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        const char *job_path = NULL, *result = NULL, *node = NULL, *unit = NULL;
        uint32_t id = 0;
        int r = 0;
        Client *client = (Client *) userdata;

        if (client->pending_job_name == NULL) {
                return 0;
        }

        r = sd_bus_message_read(m, "uosss", &id, &job_path, &node, &unit, &result);
        if (r < 0) {
                fprintf(stderr, "Can't parse job result\n");
                return 0;
        }

        (void) sd_bus_message_rewind(m, true);

        if (streq(client->pending_job_name, job_path)) {
                client->pending_job_result = sd_bus_message_ref(m);
                return 1;
        }

        return 0;
}

int method_lifecycle_action_on(Client *client, char *node_name, char *unit, char *method) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *job_result = NULL;
        char *job_path = NULL, *result = NULL, *node = NULL;
        uint32_t id = 0;
        int r = 0;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
                        HIRTE_MANAGER_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        "JobRemoved",
                        match_job_removed_signal,
                        client);

        if (r < 0) {
                fprintf(stderr, "Failed to match signal\n");
                return r;
        }

        r = sd_bus_call_method(
                        client->api_bus,
                        HIRTE_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        method,
                        &error,
                        &message,
                        "ss",
                        unit,
                        "replace");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        r = sd_bus_message_read(message, "o", &job_path);
        if (r < 0) {
                fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
                return r;
        }

        job_result = wait_for_job(client, job_path);
        if (job_result == NULL) {
                return -EIO;
        }

        r = sd_bus_message_read(job_result, "uosss", &id, &job_path, &node, &unit, &result);
        if (r < 0) {
                fprintf(stderr, "Can't parse job result\n");
                return r;
        }

        printf("Unit %s %s operation result: %s\n", unit, client->op, result);

        return r;
}

int method_metrics_toggle(Client *client, char *method) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        client->api_bus,
                        HIRTE_INTERFACE_BASE_NAME,
                        HIRTE_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        method,
                        &error,
                        &message,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        printf("Done\n");
        return r;
}

static int match_start_unit_job_metrics_signal(
                sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        const char *job_path = NULL;
        const uint64_t job_measured_time = 0;
        const uint64_t unit_net_start_time = 0;
        const char *node_name = NULL;
        const char *unit = NULL;
        int r = 0;

        r = sd_bus_message_read(
                        m, "ssstt", &node_name, &job_path, &unit, &job_measured_time, &unit_net_start_time);
        if (r < 0) {
                fprintf(stderr, "Can't parse job result: %s", strerror(-r));
                return r;
        }

        printf("[%s] Job %s to start unit %s:\n\t"
               "Hirte job gross measured time: %.1lfms\n\t"
               "Unit net start time (from properties): %.1lfms\n",
               node_name,
               job_path,
               unit,
               micros_to_millis(job_measured_time),
               micros_to_millis(unit_net_start_time));

        return 0;
}

static int match_agent_job_metrics_signal(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        const char *node_name = NULL;
        char *unit = NULL;
        char *method = NULL;
        uint64_t systemd_job_time = 0;
        int r = 0;
        r = sd_bus_message_read(m, "ssst", &node_name, &unit, &method, &systemd_job_time);
        if (r < 0) {
                fprintf(stderr, "Can't parse metric: %s", strerror(-r));
                return r;
        }

        printf("[%s] Agent systemd %s job on %s net measured time: %.1lfms\n",
               node_name,
               method,
               unit,
               micros_to_millis(systemd_job_time));
        return 0;
}

int method_metrics_listen(Client *client) {
        int r = 0;

        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s", strerror(-r));
                return r;
        }

        r = sd_bus_attach_event(client->api_bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
                        METRICS_OBJECT_PATH,
                        METRICS_INTERFACE,
                        "StartUnitJobMetrics",
                        match_start_unit_job_metrics_signal,
                        client);
        if (r < 0) {
                fprintf(stderr, "Failed to add StartUnitJobMetrics api bus match: %s", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        client->api_bus,
                        NULL,
                        HIRTE_INTERFACE_BASE_NAME,
                        METRICS_OBJECT_PATH,
                        METRICS_INTERFACE,
                        "AgentJobMetrics",
                        match_agent_job_metrics_signal,
                        client);
        if (r < 0) {
                fprintf(stderr, "Failed to add AgentJobMetrics api bus match: %s", strerror(-r));
                return r;
        }

        r = event_loop_add_shutdown_signals(event);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to agent event loop: %s", strerror(-r));
                return r;
        }

        printf("Waiting for metrics signals...\n");
        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s", strerror(-r));
                return r;
        }

        return r;
}

int client_call_manager(Client *client) {
        int r = 0;

#ifdef USE_USER_API_BUS
        r = sd_bus_open_user(&(client->api_bus));
#else
        r = sd_bus_open_system(&(client->api_bus));
#endif
        if (r < 0) {
                fprintf(stderr, "Failed to connect to api bus: %s\n", strerror(-r));
                return r;
        }

        if (streq(client->op, "help")) {
                r = print_client_usage("hirte");
        } else if (streq(client->op, "list-units")) {
                if (client->opargc == 0) {
                        r = method_list_units_on_all(client);
                } else {
                        r = method_list_units_on(client->opargv[0], client);
                }
        } else if (streq(client->op, "start")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_lifecycle_action_on(client, client->opargv[0], client->opargv[1], "StartUnit");
        } else if (streq(client->op, "stop")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_lifecycle_action_on(client, client->opargv[0], client->opargv[1], "StopUnit");
        } else if (streq(client->op, "restart")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_lifecycle_action_on(client, client->opargv[0], client->opargv[1], "RestartUnit");
        } else if (streq(client->op, "reload")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_lifecycle_action_on(client, client->opargv[0], client->opargv[1], "ReloadUnit");
        } else if (streq(client->op, "monitor")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_monitor_units_on_nodes(client->api_bus, client->opargv[0], client->opargv[1]);
        } else if (streq(client->op, "metrics")) {
                if (client->opargc != 1) {
                        return -EINVAL;
                } else if (streq(client->opargv[0], "enable")) {
                        method_metrics_toggle(client, "EnableMetrics");
                } else if (streq(client->opargv[0], "disable")) {
                        method_metrics_toggle(client, "DisableMetrics");
                } else if (streq(client->opargv[0], "listen")) {
                        method_metrics_listen(client);
                }
        } else {
                return -EINVAL;
        }

        return r;
}

int print_client_usage(char *argv) {
        printf("hirtectl is a convenience CLI tool to interact with hirte\n");
        printf("Usage: %s COMMAND\n\n", argv);
        printf("Available command:\n");
        printf("  - help: shows this help message\n");
        printf("    usage: help\n");
        printf("  - list-units: returns the list of systemd services running on a specific or on all nodes\n");
        printf("    usage: list-units [nodename]\n");
        printf("  - start: starts a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: start nodename unitname\n");
        printf("  - stop: stop a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: stop nodename unitname\n");
        printf("  - reload: reloads a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: reload nodename unitname\n");
        printf("  - restart: restarts a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: restart nodename unitname\n");
        printf("  - metrics [enable|disable]: enables/disables metrics reporting\n");
        printf("    usage: metrics [enable|disable]\n");
        printf("  - metrics listen: listen and print incoming metrics reports\n");
        printf("    usage: metrics listen\n");
        printf("  - monitor: creates a monitor on the given node to observe changes in the specified units\n");
        printf("    usage: monitor [node] [unit1,unit2,...]\n");
        return 0;
}
