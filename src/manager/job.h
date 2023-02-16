#pragma once

#include "libhirte/common/common.h"

#include "types.h"

struct Job {
        int ref_count;

        Node *node; /* weak ref */

        JobState state;

        uint32_t id;
        char *object_path;
        char *type;
        char *unit;

        sd_bus_slot *export_slot;

        uint64_t job_time_millis;

        LIST_FIELDS(Job, jobs);
};


Job *job_new(Node *node, const char *unit, const char *type);
Job *job_ref(Job *job);
void job_unref(Job *job);

void job_set_state(Job *job, JobState state);

bool job_export(Job *job);

DEFINE_CLEANUP_FUNC(Job, job_unref)
#define _cleanup_job_ _cleanup_(job_unrefp)
