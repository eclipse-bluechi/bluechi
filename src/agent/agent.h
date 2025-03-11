/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>

#include <hashmap.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"
#include "libbluechi/socket.h"

#include "types.h"

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
        AGENT_CONNECTION_STATE_RETRY,
        AGENT_CONNECTION_STATE_CONNECTING
} AgentConnectionState;

struct Agent {
        int ref_count;

        bool systemd_user;
        char *name;
        char *api_bus_service_name;

        char *host;
        int port;
        char *controller_address;
        char *assembled_controller_address;

        long heartbeat_interval_msec;
        long controller_heartbeat_threshold_msec;

        AgentConnectionState connection_state;
        uint64_t connection_retry_count;
        uint64_t controller_last_seen;
        uint64_t controller_last_seen_monotonic;
        uint64_t disconnect_timestamp;
        uint64_t disconnect_timestamp_monotonic;
        uint64_t connection_retry_count_until_quiet;

        SocketOptions *peer_socket_options;

        sd_event *event;

        sd_bus *api_bus;
        sd_bus *systemd_dbus;
        sd_bus *peer_dbus;

        sd_bus_slot *register_call_slot;

        bool metrics_enabled;
        sd_bus_slot *metrics_slot;

        LIST_HEAD(SystemdRequest, outstanding_requests);
        LIST_HEAD(JobTracker, tracked_jobs);
        LIST_HEAD(ProxyService, proxy_services);

        struct hashmap *unit_infos;
        bool wildcard_subscription_active;

        struct config *config;
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
bool agent_set_assembled_controller_address(Agent *agent, const char *address);
bool agent_set_name(Agent *agent, const char *name);
bool agent_set_heartbeat_interval(Agent *agent, const char *interval_msec);
void agent_set_systemd_user(Agent *agent, bool systemd_user);
bool agent_parse_config(Agent *agent, const char *configfile);
bool agent_apply_config(Agent *agent);

bool agent_start(Agent *agent);
void agent_stop(Agent *agent);

bool agent_is_connected(Agent *agent);
char *agent_is_online(Agent *agent);

void agent_remove_proxy(Agent *agent, ProxyService *proxy, bool emit);

int agent_send_job_metrics(Agent *agent, char *unit, char *method, uint64_t systemd_job_time);

DEFINE_CLEANUP_FUNC(Agent, agent_unref)
#define _cleanup_agent_ _cleanup_(agent_unrefp)
