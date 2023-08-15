/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <errno.h>
#include <getopt.h>

#include "libbluechi/bus/utils.h"
#include "libbluechi/common/opt.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/service/shutdown.h"

#include "client.h"
#include "method_list_units.h"
#include "method_monitor.h"
#include "method_status.h"

Client *new_client(char *op, int opargc, char **opargv, char *opt_filter_glob) {
        Client *client = malloc0(sizeof(Client));
        if (client == NULL) {
                return NULL;
        }

        client->ref_count = 1;
        client->op = op;
        client->opargc = opargc;
        client->opargv = opargv;
        client->opt_filter_glob = opt_filter_glob;

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
                        BC_INTERFACE_BASE_NAME,
                        BC_MANAGER_OBJECT_PATH,
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
                        BC_INTERFACE_BASE_NAME,
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
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
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
               "BlueChi job gross measured time: %.1lfms\n\t"
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
                        BC_INTERFACE_BASE_NAME,
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
                        BC_INTERFACE_BASE_NAME,
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

int create_message_new_method_call(
                Client *client, const char *node_name, const char *member, sd_bus_message **new_message) {
        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;

        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_new_method_call(
                        client->api_bus,
                        &outgoing_message,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        member);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new method call: %s\n", strerror(-r));
                return r;
        }

        *new_message = sd_bus_message_ref(outgoing_message);
        return 0;
}

static int add_string_array_to_message(sd_bus_message *m, char **units, size_t units_count) {
        _cleanup_free_ char **units_array = NULL;
        int r = 0;

        units_array = malloc0_array(0, sizeof(char *), units_count + 1);
        if (units_array == NULL) {
                fprintf(stderr, "Failed to allocate memory\n");
                return -ENOMEM;
        }
        size_t i = 0;
        for (i = 0; i < units_count; i++) {
                units_array[i] = units[i];
        }

        r = sd_bus_message_append_strv(m, units_array);
        if (r < 0) {
                fprintf(stderr, "Failed to append the list of units to the message: %s\n", strerror(-r));
                return r;
        }

        return 0;
}

static int parse_enable_disable_response_from_message(sd_bus_message *m) {
        int r = 0;

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "(sss)");
        if (r < 0) {
                fprintf(stderr, "Failed to open the strings array container: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                const char *op_type = NULL;
                const char *symlink_file = NULL;
                const char *symlink_dest = NULL;

                r = sd_bus_message_read(m, "(sss)", &op_type, &symlink_file, &symlink_dest);
                if (r < 0) {
                        fprintf(stderr, "Failed to read enabled unit file information: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }
                if (streq(op_type, "symlink")) {
                        fprintf(stderr, "Created symlink %s -> %s\n", symlink_file, symlink_dest);
                } else if (streq(op_type, "unlink")) {
                        fprintf(stderr, "Removed \"%s\".\n", symlink_file);
                } else {
                        fprintf(stderr, "Unknown operation: %s\n", op_type);
                }
        }

        return 0;
}

static int method_enable_unit_on(
                Client *client, char *node_name, char **units, size_t units_count, int runtime, int force) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;

        r = create_message_new_method_call(client, node_name, "EnableUnitFiles", &outgoing_message);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new message: %s\n", strerror(-r));
                return r;
        }

        r = add_string_array_to_message(outgoing_message, units, units_count);
        if (r < 0) {
                fprintf(stderr, "Failed to append the string array to the message: %s\n", strerror(-r));
        }

        r = sd_bus_message_append(outgoing_message, "bb", runtime, force);
        if (r < 0) {
                fprintf(stderr, "Failed to append runtime and force to the message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, outgoing_message, BC_DEFAULT_DBUS_TIMEOUT, &error, &result);
        if (r < 0) {
                fprintf(stderr, "Failed to issue call: %s\n", error.message);
                return r;
        }

        int carries_install_info = 0;
        r = sd_bus_message_read(result, "b", &carries_install_info);
        if (r < 0) {
                fprintf(stderr, "Failed to read carries_install_info from the message: %s\n", strerror(-r));
                return r;
        }

        if (carries_install_info) {
                fprintf(stderr, "The unit files included enablement information\n");
        } else {
                fprintf(stderr, "The unit files did not include any enablement information\n");
        }

        r = parse_enable_disable_response_from_message(result);
        if (r < 0) {
                fprintf(stderr, "Failed to parse the response strings array: %s\n", error.message);
                return r;
        }

        return 0;
}

static int method_disable_unit_on(Client *client, char *node_name, char **units, size_t units_count, int runtime) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;

        r = create_message_new_method_call(client, node_name, "DisableUnitFiles", &outgoing_message);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new message: %s\n", strerror(-r));
                return r;
        }

        r = add_string_array_to_message(outgoing_message, units, units_count);
        if (r < 0) {
                fprintf(stderr, "Failed to append the string array to the message: %s\n", strerror(-r));
        }

        r = sd_bus_message_append(outgoing_message, "b", runtime);
        if (r < 0) {
                fprintf(stderr, "Failed to append runtime to the message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, outgoing_message, BC_DEFAULT_DBUS_TIMEOUT, &error, &result);
        if (r < 0) {
                fprintf(stderr, "Failed to issue call: %s\n", error.message);
                return r;
        }

        r = parse_enable_disable_response_from_message(result);
        if (r < 0) {
                fprintf(stderr, "Failed to parse the response strings array: %s\n", error.message);
                return r;
        }

        return 0;
}

static int method_daemon_reload_on(Client *client, char *node_name) {
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
                        "Reload",
                        &error,
                        &result,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        return 0;
}

static int method_set_log_level(Client *client, char *node_name, char *loglevel) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        if (node_name != NULL) {
                char *path = NULL;
                int r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &path);
                if (r < 0) {
                        r = assemble_object_path_string(
                                        NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
                        if (r < 0) {
                                fprintf(stderr, "Failed to assemble pbject path to string: %s\n", strerror(-r));
                                return r;
                        }
                }

                r = sd_bus_call_method(
                                client->api_bus,
                                BC_INTERFACE_BASE_NAME,
                                path,
                                NODE_INTERFACE,
                                "SetLogLevel",
                                &error,
                                &result,
                                "s",
                                loglevel);
                if (r < 0) {
                        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                        return r;
                }
                return 0;
        } else {
                r = sd_bus_call_method(
                                client->api_bus,
                                BC_INTERFACE_BASE_NAME,
                                BC_OBJECT_PATH,
                                MANAGER_INTERFACE,
                                "SetLogLevel",
                                &error,
                                &result,
                                "s",
                                loglevel);
                if (r < 0) {
                        fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                        return r;
                }
        }
        return 0;
}

static int method_freeze_unit_on(Client *client, char *node_name, char *unit) {
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
                        "FreezeUnit",
                        &error,
                        &result,
                        "s",
                        unit);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        printf("Unit %s %s operation done\n", unit, client->op);

        return 0;
}

static int method_thaw_unit_on(Client *client, char *node_name, char *unit) {
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
                        "ThawUnit",
                        &error,
                        &result,
                        "s",
                        unit);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                return r;
        }

        printf("Unit %s %s operation done\n", unit, client->op);

        return 0;
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
                r = print_client_usage("bluechi");
        } else if (streq(client->op, "list-units")) {
                if (client->opargc == 0) {
                        r = method_list_units_on_all(
                                        client->api_bus, print_unit_list_simple, client->opt_filter_glob);
                        return r;
                }

                char *node_name = client->opargv[0];
                return method_list_units_on(
                                client->api_bus, node_name, print_unit_list_simple, client->opt_filter_glob);

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
        } else if (streq(client->op, "freeze")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_freeze_unit_on(client, client->opargv[0], client->opargv[1]);
        } else if (streq(client->op, "thaw")) {
                if (client->opargc != 2) {
                        return -EINVAL;
                }
                r = method_thaw_unit_on(client, client->opargv[0], client->opargv[1]);
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
                /* monitor node connection status changes */
                if (client->opargc == 1 && streq(client->opargv[0], "node-connection")) {
                        return method_monitor_node_connection_state(client->api_bus);
                }

                /* monitor systemd units on the specified nodes */
                char *arg0 = SYMBOL_WILDCARD;
                char *arg1 = SYMBOL_WILDCARD;
                switch (client->opargc) {
                case 0:
                        break;
                case 1:
                        arg0 = client->opargv[0];
                        break;
                case 2:
                        arg0 = client->opargv[0];
                        arg1 = client->opargv[1];
                        break;
                default:
                        return -EINVAL;
                }
                r = method_monitor_units_on_nodes(client->api_bus, arg0, arg1);
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
        } else if (streq(client->op, "enable")) {
                if (client->opargc < 2) {
                        return -EINVAL;
                }
                r = method_enable_unit_on(
                                client, client->opargv[0], &client->opargv[1], client->opargc - 1, 0, 0);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to enable the units on node [%s] - %s",
                                client->opargv[0],
                                strerror(-r));
                } else {
                        r = method_daemon_reload_on(client, client->opargv[0]);
                }
        } else if (streq(client->op, "disable")) {
                if (client->opargc < 2) {
                        return -EINVAL;
                }
                r = method_disable_unit_on(
                                client, client->opargv[0], &client->opargv[1], client->opargc - 1, 0);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to disable the units on node [%s] - %s",
                                client->opargv[0],
                                strerror(-r));
                } else {
                        r = method_daemon_reload_on(client, client->opargv[0]);
                }
        } else if (streq(client->op, "daemon-reload")) {
                if (client->opargc != 1) {
                        return -EINVAL;
                }
                r = method_daemon_reload_on(client, client->opargv[0]);
        } else if (streq(client->op, "status")) {
                if (client->opargc < 2) {
                        return -EINVAL;
                }
                r = method_status_unit_on(client, client->opargv[0], &client->opargv[1], client->opargc - 1);
        } else if (streq(client->op, "set-loglevel")) {
                if (client->opargc == 1) {
                        r = method_set_log_level(client, NULL, client->opargv[0]);
                } else if (client->opargc == 2) {
                        r = method_set_log_level(client, client->opargv[0], client->opargv[1]);
                } else {
                        return -EINVAL;
                }
        } else if (streq(client->op, "version")) {
                printf("%s\n", CONFIG_H_BC_VERSION);
        } else {
                return -EINVAL;
        }

        return r;
}

int print_client_usage(char *argv) {
        printf("bluechictl is a convenience CLI tool to interact with bluechi\n");
        printf("Usage: %s COMMAND\n\n", argv);
        printf("Available command:\n");
        printf("  - help: shows this help message\n");
        printf("    usage: help\n");
        printf("  - list-units: returns the list of systemd services running on a specific or on all nodes\n");
        printf("    usage: list-units [nodename] [--filter=glob]\n");
        printf("  - start: starts a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: start nodename unitname\n");
        printf("  - stop: stop a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: stop nodename unitname\n");
        printf("  - freeze: freeze a specific systemd service on a specific node\n");
        printf("    usage: freeze nodename unitname\n");
        printf("  - thaw: thaw a frozen systemd service on a specific node\n");
        printf("    usage: thaw nodename unitname\n");
        printf("  - reload: reloads a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: reload nodename unitname\n");
        printf("  - restart: restarts a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: restart nodename unitname\n");
        printf("  - enable: enables the specified systemd files on a specific node\n");
        printf("    usage: enable nodename unitfilename...\n");
        printf("  - disable: disables the specified systemd files on a specific node\n");
        printf("    usage: disable nodename unitfilename...\n");
        printf("  - metrics [enable|disable]: enables/disables metrics reporting\n");
        printf("    usage: metrics [enable|disable]\n");
        printf("  - metrics listen: listen and print incoming metrics reports\n");
        printf("    usage: metrics listen\n");
        printf("  - monitor: creates a monitor on the given node to observe changes in the specified units\n");
        printf("    usage: monitor [node] [unit1,unit2,...]\n");
        printf("  - monitor node-connection: creates a monitor to observe changes in state of all nodes\n");
        printf("    usage: monitor node-connection\n");
        printf("  - daemon-reload: reload systemd daemon on a specific node\n");
        printf("    usage: disable nodename\n");
        return 0;
}
