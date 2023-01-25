#include <stdio.h>

#include "../include/job-manager.h"
#include "common/memory.h"

JobManager *job_manager_new() {
        fprintf(stdout, "Creating Job Manager...\n");

        JobManager *mgr = malloc0(sizeof(JobManager));
        LIST_HEAD_INIT(mgr->head);

        return mgr;
}

void job_manager_unrefp(JobManager **manager) {
        if (manager == NULL || (*manager) == NULL) {
                return;
        }

        free(*manager);
}