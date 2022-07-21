#include "types.h"

static const char* const job_type_table[_JOB_TYPE_MAX] = {
        [JOB_ISOLATE_ALL] = "isolate-all",
};

const char *job_type_to_string(JobType type) {
        return ENUM_TO_STRING(type, job_type_table);
}

static const char* const node_job_type_table[_JOB_TYPE_MAX] = {
        [NODE_JOB_ISOLATE] = "isolate",
};

const char *node_job_type_to_string(NodeJobType type) {
        return ENUM_TO_STRING(type, node_job_type_table);
}

const char* const job_state_table[_JOB_STATE_MAX] = {
        [JOB_WAITING] = "waiting",
        [JOB_RUNNING] = "running",
};

const char *job_state_to_string(JobState state) {
        return ENUM_TO_STRING(state, job_state_table);
}

static const char* const job_result_table[_JOB_RESULT_MAX] = {
        [JOB_DONE] = "done",
        [JOB_CANCELED] = "canceled",
        [JOB_FAILED] = "failed",
};

const char *job_result_to_string(JobResult result) {
        return ENUM_TO_STRING(result, job_result_table);
}

Job *job_new(Manager *manager, int job_type, size_t job_size) {
        _cleanup_free_ Job *job = NULL;
        _cleanup_free_ char *object_path = NULL;
        uint32_t id;
        int r;

        job = malloc0(job_size);
        if (job == NULL)
                return NULL;

        id = ++manager->next_job_id;
        r = asprintf(&object_path, "%s/%d", manager->job_path_prefix, id);
        if (r < 0)
                return NULL;

        job->type = job_type;
        job->id = id;
        job->object_path = steal_pointer (&object_path);
        job->state = JOB_WAITING;
        job->manager = manager;
        job->ref_count = 1;
        LIST_INIT(jobs, job);

        return steal_pointer(&job);
}

Job *job_ref(Job *job) {
        job->ref_count++;
        return job;
}

void job_unref(Job *job) {
        job->ref_count--;

        if (job->ref_count == 0) {
                printf ("Freeing job %p\n", job);
                if (job->destroy_cb)
                        job->destroy_cb(job);

                if (job->source_message)
                        sd_bus_message_unref (job->source_message);
                free(job->object_path);
                if (job->bus_slot)
                        sd_bus_slot_unref(job->bus_slot);
                free(job);
        }
}


static void manager_add_job(Manager *manager, Job *job) {
        LIST_APPEND(jobs, manager->jobs, job_ref(job));
}

static void manager_remove_job(Manager *manager, Job *job) {
        LIST_REMOVE(jobs, manager->jobs, job);
        job_unref(job);
}

static int manager_send_job_new_signal(Manager *manager, Job *job) {
        _cleanup_(sd_bus_message_unrefp) sd_bus_message *m = NULL;
        int r;

        r = sd_bus_message_new_signal(
                        manager->bus,
                        &m,
                        manager->manager_path,
                        manager->manager_iface,
                        "JobNew");
        if (r < 0)
                return r;

        r = sd_bus_message_append(m, "uo", job->id, job->object_path);
        if (r < 0)
                return r;

        return sd_bus_send(manager->bus, m, NULL);
}

static int manager_send_job_removed_signal(Manager *manager, Job *job) {
        _cleanup_(sd_bus_message_unrefp) sd_bus_message *m = NULL;
        _cleanup_free_ char *p = NULL;
        int r;

        r = sd_bus_message_new_signal(
                        manager->bus,
                        &m,
                        manager->manager_path,
                        manager->manager_iface,
                        "JobRemoved");
        if (r < 0)
                return r;

        r = sd_bus_message_append(m, "uos", job->id, job->object_path, job_result_to_string(job->result));
        if (r < 0)
                return r;

        return sd_bus_send(manager->bus, m, NULL);
}

/* Only called from mainloop */
static void try_start_job (Manager *manager) {
        Job *job;

        assert(manager->job_source == NULL);
        assert(manager->current_job == NULL);

        job = manager->jobs;
        if (job == NULL)
                return;

        manager->current_job = job_ref(job);

        job->state = JOB_RUNNING;
        sd_bus_emit_properties_changed(manager->bus, job->object_path, JOB_IFACE, "State", NULL);

        (job->start_cb)(job, job->userdata);
}

static int finish_job_cb (sd_event_source *s, void *userdata) {
        Manager *manager = userdata;
        _cleanup_(job_unrefp) Job *job = NULL;

        job = steal_pointer (&manager->current_job);
        assert (job != NULL);

        manager_send_job_removed_signal(manager, job);

        manager_remove_job(manager, job);

        sd_event_source_unref (manager->job_source);
        manager->job_source = NULL;

        try_start_job(manager);

        return 0;
}

void manager_finish_job(Manager *manager, Job *job) {
        int r;

        printf("Finish job %p\n", job);

        assert (manager->current_job == job);
        assert (manager->job_source == NULL);

        /* Kick off finish job in mainloop */
        r = sd_event_add_defer(manager->event, &manager->job_source, finish_job_cb, manager);
        if (r < 0) {
                fprintf(stderr, "No memory to queue job scheduler");
        }
}

static int start_job_cb (sd_event_source *s, void *userdata) {
        Manager *manager = userdata;

        sd_event_source_unref (manager->job_source);
        manager->job_source = NULL;

        try_start_job(manager);

        return 0;
}

static void schedule_job(Manager *manager) {
        int r;

        if (manager->current_job || manager->job_source)
                return; /* Job already running or scheduled */

        if (manager->jobs == NULL)
                return; /* No jobs */

        /* Kick of job */
        r = sd_event_add_defer(manager->event, &manager->job_source, start_job_cb, manager);
        if (r < 0) {
                fprintf(stderr, "No memory to queue job scheduler");
        }

        printf ("Scheduled job start\n");
}

static BUS_DEFINE_PROPERTY_GET_ENUM(property_get_type, job_type, JobType);
static BUS_DEFINE_PROPERTY_GET_ENUM(property_get_state, job_state, JobState);

static const sd_bus_vtable job_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_PROPERTY("JobType", "s", property_get_type, offsetof(Job, type), SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("State", "s", property_get_state, offsetof(Job, state), SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
        SD_BUS_VTABLE_END
};

int manager_queue_job(Manager *manager,
                      int job_type,
                      size_t job_size,
                      job_start_callback start_cb,
                      job_cancel_callback cancel_cb,
                      job_destroy_callback destroy_cb,
                      void *userdata,
                      Job **job_out) {
        _cleanup_(job_unrefp) Job *job = NULL;
        int r;

        job = job_new(manager, job_type, job_size);
        if (job == NULL) {
                return -ENOMEM;
        }

        job->start_cb = start_cb;
        job->cancel_cb = cancel_cb;
        job->destroy_cb = destroy_cb;
        job->userdata = userdata;

        r = sd_bus_add_object_vtable(manager->bus,
                                     &job->bus_slot,
                                     job->object_path,
                                     JOB_IFACE,
                                     job_vtable,
                                     job);
        if (r < 0) {
                fprintf(stderr, "Failed to add job bus vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        if (job_out)
                *job_out = job_ref(job);

        manager_add_job(job->manager, job);
        manager_send_job_new_signal(manager, job);

        printf ("Queued job %d\n", job->id);

        schedule_job(manager);

        return 0;
}
