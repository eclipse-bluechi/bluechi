#include <stddef.h>

#include "job.h"
#include "manager.h"
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
                return NULL;
        }

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

        free(job->object_path);
        free(job->unit);
        free(job->type);
        free(job);
}

bool job_export(Job *job) {
        Node *node = job->node;
        Manager *manager = node->manager;

        int r = sd_bus_add_object_vtable(
                        manager->user_dbus, &job->export_slot, job->object_path, JOB_INTERFACE, job_vtable, job);
        if (r < 0) {
                fprintf(stderr, "Failed to add job vtable: %s\n", strerror(-r));
                return false;
        }

        return true;
}

void job_set_state(Job *job, JobState state) {
        Node *node = job->node;
        Manager *manager = node->manager;

        job->state = state;

        int r = sd_bus_emit_properties_changed(
                        manager->user_dbus, job->object_path, JOB_INTERFACE, "State", NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to emit status property changed: %s\n", strerror(-r));
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

static int job_method_cancel(UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        /* TODO: Implement */
        return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Not implemented yet");
}
