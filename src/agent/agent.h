#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "libhirte/common/common.h"
#include "libhirte/hashmap/hashmap.h"

typedef struct Agent Agent;
typedef struct SystemdRequest SystemdRequest;

struct SystemdRequest {
        int ref_count;
        Agent *agent;

        sd_bus_message *request_message;

        sd_bus_slot *slot;

        sd_bus_message *message;

        void *userdata;
        free_func_t free_userdata;

        LIST_FIELDS(SystemdRequest, outstanding_requests);
};

SystemdRequest *systemd_request_ref(SystemdRequest *req);
void systemd_request_unref(SystemdRequest *req);

DEFINE_CLEANUP_FUNC(SystemdRequest, systemd_request_unref)
#define _cleanup_systemd_request_ _cleanup_(systemd_request_unrefp)

typedef void (*job_tracker_callback)(sd_bus_message *m, const char *result, void *userdata);
typedef struct JobTracker JobTracker;

typedef enum {
        AGENT_CONNECTION_STATE_DISCONNECTED,
        AGENT_CONNECTION_STATE_CONNECTED,
        AGENT_CONNECTION_STATE_RETRY
} AgentConnectionState;

struct Agent {
        int ref_count;

        char *name;
        char *host;
        int port;

        AgentConnectionState connection_state;
        uint64_t connection_retry_count;

        char *orch_addr;
        char *api_bus_service_name;

        sd_event *event;

        sd_bus *api_bus;
        sd_bus *systemd_dbus;
        sd_bus *peer_dbus;

        LIST_HEAD(SystemdRequest, outstanding_requests);

        LIST_HEAD(JobTracker, tracked_jobs);

        struct hashmap *unit_infos;
};

typedef struct AgentUnitInfo {
        char *object_path; /* key */
        char *unit;
        bool subscribed;
        bool loaded;
        UnitActiveState active_state;
        char *substate;
} AgentUnitInfo;


Agent *agent_new(void);
Agent *agent_ref(Agent *agent);
void agent_unref(Agent *agent);

bool agent_set_port(Agent *agent, const char *port);
bool agent_set_host(Agent *agent, const char *host);
bool agent_set_name(Agent *agent, const char *name);
bool agent_parse_config(Agent *agent, const char *configfile);

bool agent_start(Agent *agent);
bool agent_stop(Agent *agent);

DEFINE_CLEANUP_FUNC(Agent, agent_unref)
#define _cleanup_agent_ _cleanup_(agent_unrefp)
