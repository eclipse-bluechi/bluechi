/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stddef.h>

#include "controller.h"
#include "job.h"
#include "libbluechi/log/log.h"
#include "node.h"

static int job_property_get_nodename(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);
static int job_property_get_state(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);
static int job_method_cancel(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

static const sd_bus_vtable job_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Cancel", "", "", job_method_cancel, 0),
        SD_BUS_PROPERTY("Id", "u", NULL, offsetof(Job, id), SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("Node", "s", job_property_get_nodename, 0, SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("Unit", "s", NULL, offsetof(Job, unit), SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("JobType", "s", NULL, offsetof(Job, type), SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("State", "s", job_property_get_state, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
        SD_BUS_VTABLE_END
};

Job *job_new(Node *node, const char *unit, const char *type) {
        static uint32_t next_id = 0;

        _cleanup_job_ Job *job = malloc0(sizeof(Job));
        if (job == NULL) {
                return NULL;
        }

        job->ref_count = 1;
        job->state = JOB_WAITING;
        job->id = ++next_id;
        job->node = node;
        LIST_INIT(jobs, job);

        job->type = strdup(type);
        if (job->type == NULL) {
                return NULL;
        }

        job->unit = strdup(unit);
        if (job->unit == NULL) {
                return NULL;
        }

        int r = asprintf(&job->object_path, "%s/%u", JOB_OBJECT_PATH_PREFIX, job->id);
        if (r < 0) {
                bc_log_error("Out of memory");
                return NULL;
        }

        job->job_start_micros = 0;
        job->job_end_micros = 0;

        return steal_pointer(&job);
}

Job *job_ref(Job *job) {
        job->ref_count++;
        return job;
}

void job_unref(Job *job) {
        job->ref_count--;
        if (job->ref_count != 0) {
                return;
        }

        sd_bus_slot_unrefp(&job->export_slot);

        free_and_null(job->object_path);
        free_and_null(job->unit);
        free_and_null(job->type);
        free(job);
}

bool job_export(Job *job) {
        Node *node = job->node;
        Controller *controller = node->controller;

        int r = sd_bus_add_object_vtable(
                        controller->api_bus, &job->export_slot, job->object_path, JOB_INTERFACE, job_vtable, job);
        if (r < 0) {
                bc_log_errorf("Failed to add job vtable: %s", strerror(-r));
                return false;
        }

        return true;
}

void job_set_state(Job *job, JobState state) {
        Node *node = job->node;
        Controller *controller = node->controller;

        job->state = state;

        int r = sd_bus_emit_properties_changed(
                        controller->api_bus, job->object_path, JOB_INTERFACE, "State", NULL);
        if (r < 0) {
                bc_log_errorf("Failed to emit status property changed: %s", strerror(-r));
        }
}

static int job_property_get_nodename(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Job *job = userdata;
        Node *node = job->node;

        return sd_bus_message_append(reply, "s", node->name);
}

static int job_property_get_state(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Job *job = userdata;

        return sd_bus_message_append(reply, "s", job_state_to_string(job->state));
}


static int job_cancel_callback(UNUSED AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Cancelling job failed: %s", sd_bus_message_get_error(m)->message);
                return 0;
        }

        bc_log_debug("Job canceled successfully");
        return 0;
}

static int job_method_cancel(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Job *job = (Job *) userdata;

        _cleanup_agent_request_ AgentRequest *req = NULL;
        int r = node_create_request(
                        &req,
                        job->node,
                        "JobCancel",
                        job_cancel_callback,
                        sd_bus_message_ref(m),
                        (free_func_t) sd_bus_message_unref);
        if (req == NULL) {
                sd_bus_message_unref(m);
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create an agent request to cancel job: %s",
                                strerror(-r));
        }

        r = sd_bus_message_append(req->message, "u", job->id);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to append job ID to cancel message: %s",
                                strerror(-r));
        }

        r = agent_request_start(req);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to call the method to cancel job '%d': %s",
                                job->id,
                                strerror(-r));
        }

        /* Cancel request submitted successfully, so directly reply here
         * to avoid a race condition where the job canceling has been
         * processed faster than the job_cancel_callback - which leads to
         * an unknown method Cancel or interface org.eclipse.bluechi.Job reply
         */
        return sd_bus_reply_method_return(m, "");
}
