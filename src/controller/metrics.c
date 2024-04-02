/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libbluechi/common/common.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/log/log.h"

#include "controller.h"
#include "job.h"
#include "metrics.h"
#include "node.h"

static const sd_bus_vtable metrics_api_vtable[] = {
        SD_BUS_VTABLE_START(0),

        SD_BUS_SIGNAL_WITH_NAMES(
                        "StartUnitJobMetrics",
                        "ssstt",
                        SD_BUS_PARAM(node_name) SD_BUS_PARAM(job_id) SD_BUS_PARAM(unit) SD_BUS_PARAM(
                                        job_measured_time_micros) SD_BUS_PARAM(unit_start_prop_time_micros),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "AgentJobMetrics",
                        "ssst",
                        SD_BUS_PARAM(node_name) SD_BUS_PARAM(unit) SD_BUS_PARAM(method)
                                        SD_BUS_PARAM(systemd_job_time_micros),
                        0),
        SD_BUS_VTABLE_END
};

int metrics_export(Controller *controller) {
        int r = sd_bus_add_object_vtable(
                        controller->api_bus,
                        &controller->metrics_slot,
                        METRICS_OBJECT_PATH,
                        METRICS_INTERFACE,
                        metrics_api_vtable,
                        NULL);
        if (r < 0) {
                bc_log_errorf("Failed to add API metrics vtable: %s", strerror(-r));
                return r;
        }

        return 0;
}

static int node_metrics_match_agent_job(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        char *unit = NULL;
        char *method = NULL;
        uint64_t systemd_job_time = 0;
        int r = sd_bus_message_read(m, "sst", &unit, &method, &systemd_job_time);
        if (r < 0) {
                bc_log_errorf("Invalid generic int64 metric signal: %s", strerror(-r));
                return r;
        }
        bc_log_debugf("Reporting agent %s job metrics on unit %s: %ldus for node %s",
                      unit,
                      method,
                      systemd_job_time,
                      node->name);
        r = sd_bus_emit_signal(
                        node->controller->api_bus,
                        METRICS_OBJECT_PATH,
                        METRICS_INTERFACE,
                        "AgentJobMetrics",
                        "ssst",
                        node->name,
                        unit,
                        method,
                        systemd_job_time);
        if (r < 0) {
                bc_log_errorf("Failed to emit StartUnitJobMetrics signal: %s", strerror(-r));
                return r;
        }

        return 0;
}

bool metrics_node_signal_matching_register(Node *node) {
        int r = sd_bus_match_signal(
                        node->agent_bus,
                        &node->metrics_matching_slot,
                        NULL,
                        INTERNAL_AGENT_METRICS_OBJECT_PATH,
                        INTERNAL_AGENT_METRICS_INTERFACE,
                        "AgentJobMetrics",
                        node_metrics_match_agent_job,
                        node);
        if (r < 0) {
                bc_log_errorf("Failed to add metrics signal matching: %s", strerror(-r));
                return false;
        }

        return true;
}

void metrics_produce_job_report(Job *job) {
        int r = 0;
        uint64_t inactive_exit_timestamp = 0, active_enter_timestamp = 0, unit_net_start_time_micros = 0;
        uint64_t job_measured_time_micros = 0;
        r = node_method_get_unit_uint64_property_sync(
                        job->node, job->unit, "InactiveExitTimestampMonotonic", &inactive_exit_timestamp);
        if (r < 0) {
                bc_log_errorf("Failed to get unit property InactiveExitTimestampMonotonic: %s", strerror(-r));
                return;
        }

        r = node_method_get_unit_uint64_property_sync(
                        job->node, job->unit, "ActiveEnterTimestampMonotonic", &active_enter_timestamp);
        if (r < 0) {
                bc_log_errorf("Failed to get unit property ActiveEnterTimestampMonotonic: %s", strerror(-r));
                return;
        }

        unit_net_start_time_micros = (active_enter_timestamp - inactive_exit_timestamp);
        job_measured_time_micros = job->job_end_micros - job->job_start_micros;

        bc_log_debugf("Reporting job metrics: Job measured time: %ldus, Unit start time from properties: %ldus",
                      job_measured_time_micros,
                      unit_net_start_time_micros);

        r = sd_bus_emit_signal(
                        job->node->controller->api_bus,
                        METRICS_OBJECT_PATH,
                        METRICS_INTERFACE,
                        "StartUnitJobMetrics",
                        "ssstt",
                        job->node->name,
                        job->object_path,
                        job->unit,
                        job_measured_time_micros,
                        unit_net_start_time_micros);
        if (r < 0) {
                bc_log_errorf("Failed to emit StartUnitJobMetrics signal: %s", strerror(-r));
        }
}
