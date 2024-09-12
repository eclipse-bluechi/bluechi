/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-metrics.h"
#include "client.h"
#include "usage.h"

#include "libbluechi/common/string-util.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/service/shutdown.h"

static int method_metrics_toggle(Client *client, char *method) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        int r = 0;

        r = sd_bus_call_method(
                        client->api_bus,
                        BC_INTERFACE_BASE_NAME,
                        BC_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
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
        fflush(stdout);

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
        fflush(stdout);
        return 0;
}

static int method_metrics_listen(Client *client) {
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

        r = event_loop_add_shutdown_signals(event, NULL);
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

int method_metrics(Command *command, void *userdata) {
        if (streq(command->opargv[0], "enable")) {
                return method_metrics_toggle(userdata, "EnableMetrics");
        } else if (streq(command->opargv[0], "disable")) {
                return method_metrics_toggle(userdata, "DisableMetrics");
        } else if (streq(command->opargv[0], "listen")) {
                return method_metrics_listen(userdata);
        } else {
                fprintf(stderr, "Unknown metrics command: %s", command->opargv[0]);
                return -EINVAL;
        }
}

void usage_method_metrics() {
        usage_print_header();
        usage_print_description("View metrics for start/stop systemd units via BlueChi");
        usage_print_usage("bluechictl metrics [enable|disable|listen]");
        printf("\n");
        printf("Examples:\n");
        printf("  bluechictl metrics enable\n");
        printf("  bluechictl metrics listen\n");
}
