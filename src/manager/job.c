#include "job.h"
#include "manager.h"
#include "node.h"

static const sd_bus_vtable job_vtable[] = { SD_BUS_VTABLE_START(0), SD_BUS_VTABLE_END };

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
