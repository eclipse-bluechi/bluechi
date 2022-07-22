#include "orch.h"

typedef enum JobType JobType;
typedef enum NodeJobType NodeJobType;
typedef enum JobState JobState;
typedef enum JobResult JobResult;

enum JobType {
        JOB_ISOLATE_ALL,
        _JOB_TYPE_MAX,
        _JOB_TYPE_INVALID = -1
};

enum NodeJobType {
        NODE_JOB_ISOLATE,
        _NODE_JOB_TYPE_MAX,
        _NODE_JOB_TYPE_INVALID = -1
};

enum JobState {
        JOB_WAITING,
        JOB_RUNNING,
        _JOB_STATE_MAX,
        _JOB_STATE_INVALID = -1
};

enum JobResult {
        JOB_DONE,                /* Job completed successfully */
        JOB_CANCELED,            /* Job canceled by explicit cancel request */
        JOB_FAILED,              /* Job failed */
        _JOB_RESULT_MAX,
        _JOB_RESULT_INVALID = -1
};

extern const char *job_type_to_string(JobType type);
extern const char *node_job_type_to_string(NodeJobType type);
extern const char *job_state_to_string(JobState state);
extern const char *job_result_to_string(JobResult result);


typedef struct Manager Manager;
typedef struct Job Job;
typedef struct JobTracker JobTracker;

typedef void (*job_tracker_callback)(sd_bus_message *m, const char *result, void *userdata);

struct JobTracker {
        const char *object_path;
        job_tracker_callback callback;
        void *userdata;
        LIST_FIELDS(JobTracker, trackers);
};

typedef int (*job_start_callback)(Job *job);
typedef int (*job_cancel_callback)(Job *job);
typedef void (*job_destroy_callback)(Job *job);

struct Job {
        int ref_count;
        int type;
        JobState state;
        JobResult result;
        Manager *manager;
        sd_bus_slot *bus_slot;
        uint32_t id;
        char *object_path;

        sd_bus_message *source_message;

        job_start_callback start_cb;
        job_cancel_callback cancel_cb;
        job_destroy_callback destroy_cb;

        LIST_FIELDS(Job, jobs);
};

struct Manager {
        sd_event *event;
        sd_bus *bus;           /* system bus for orchestrator, peer bus for node */
        uint32_t next_job_id;
        char *job_path_prefix;
        char *manager_path;
        char *manager_iface;

        Job *current_job;
        sd_event_source *job_source;
        LIST_HEAD(Job, jobs);
};


extern Job *job_new(Manager *manager, int job_type, size_t job_size);
extern Job *job_ref(Job *job);
extern void job_unref(Job *job);
_SD_DEFINE_POINTER_CLEANUP_FUNC(Job, job_unref);

void manager_finish_job(Manager *manager, Job *job);
int manager_queue_job(Manager *manager,
                      int job_type,
                      size_t job_size,
                      sd_bus_message *source_message,
                      job_start_callback start_cb,
                      job_cancel_callback cancel_cb,
                      job_destroy_callback destroy_cb,
                      Job **job_out);
