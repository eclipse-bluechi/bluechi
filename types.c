#include "types.h"

static const char* const job_type_table[_JOB_TYPE_MAX] = {
        [JOB_ISOLATE_ALL] = "isolate-all",
};

const char *job_type_to_string(JobType type) {
        return ENUM_TO_STRING(type, job_type_table);
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
