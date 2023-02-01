#pragma once

typedef struct Manager Manager;
typedef struct Node Node;
typedef struct AgentRequest AgentRequest;
typedef struct Job Job;

typedef enum JobState {
        JOB_WAITING,
        JOB_RUNNING,
} JobState;
