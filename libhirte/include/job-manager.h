#pragma once

#include "../include/common/list.h"
#include "../include/common/memory.h"

typedef struct {

} Job;

typedef struct JobListItem {
        LIST_FIELDS(struct JobListItem, jobs);
} JobListItem;

typedef struct {
        LIST_HEAD(JobListItem, head);
} JobManager;

JobManager *job_manager_new();
void job_manager_unrefp(JobManager **manager);

#define _cleanup_job_manager_ _cleanup_(job_manager_unrefp)