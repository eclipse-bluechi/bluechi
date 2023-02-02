#include "common.h"

const char *job_state_to_string(JobState s) {
        switch (s) {
        case JOB_WAITING:
                return "waiting";
        case JOB_RUNNING:
                return "running";
        default:
                return "invalid";
        }
}
JobState job_state_from_string(const char *s) {
        if (streq(s, "waiting")) {
                return JOB_WAITING;
        }
        if (streq(s, "running")) {
                return JOB_RUNNING;
        }
        return JOB_INVALID;
}
