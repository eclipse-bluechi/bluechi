#pragma once

typedef struct Manager Manager;
typedef struct Node Node;
typedef struct AgentRequest AgentRequest;
typedef struct Job Job;

typedef enum JobState {
        JOB_WAITING,
        JOB_RUNNING,
} JobState;

static inline const char *job_state_to_string(JobState s) {
        switch (s) {
        case JOB_WAITING:
                return "waiting";
        case JOB_RUNNING:
                return "running";
        default:
                return "invalid";
        }
}
