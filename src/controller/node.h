/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <hashmap.h>

#include "libbluechi/common/common.h"

#include "types.h"

typedef int (*agent_request_response_t)(AgentRequest *req, sd_bus_message *m, sd_bus_error *ret_error);

struct AgentRequest {
        int ref_count;
        Node *node;

        sd_bus_slot *slot;

        sd_bus_message *message;

        void *userdata;
        free_func_t free_userdata;
        agent_request_response_t cb;

        bool is_cancelled;

        LIST_FIELDS(AgentRequest, outstanding_requests);
};

AgentRequest *agent_request_ref(AgentRequest *req);
void agent_request_unref(AgentRequest *req);

int agent_request_start(AgentRequest *req);

int agent_request_cancel(AgentRequest *r);

struct Node {
        int ref_count;

        Controller *controller; /* weak ref */

        /* public bus api */
        sd_bus_slot *export_slot;

        /* internal bus api */
        sd_bus *agent_bus;
        sd_bus_slot *internal_controller_slot;
        sd_bus_slot *disconnect_slot;
        sd_bus_slot *metrics_matching_slot;

        LIST_FIELDS(Node, nodes);

        char *name; /* NULL for not yet registered nodes */
        char *object_path;
        char *peer_ip;

        LIST_HEAD(AgentRequest, outstanding_requests);
        LIST_HEAD(ProxyMonitor, proxy_monitors);
        LIST_HEAD(ProxyDependency, proxy_dependencies);

        struct hashmap *unit_subscriptions;
        uint64_t last_seen;

        bool is_shutdown;
};

Node *node_new(Controller *controller, const char *name);
Node *node_ref(Node *node);
void node_unref(Node *node);
void node_shutdown(Node *node);
void node_disconnect(Node *node);

const char *node_get_status(Node *node);

bool node_export(Node *node);
bool node_has_agent(Node *node);
bool node_is_online(Node *node);
bool node_set_agent_bus(Node *node, sd_bus *bus);
void node_unset_agent_bus(Node *node);

AgentRequest *node_request_list_units(
                Node *node, agent_request_response_t cb, void *userdata, free_func_t free_userdata);
AgentRequest *node_request_list_unit_files(
                Node *node, agent_request_response_t cb, void *userdata, free_func_t free_userdata);

void node_subscribe(Node *node, Subscription *sub);
void node_unsubscribe(Node *node, Subscription *sub);

int node_add_proxy_dependency(Node *node, const char *unit_name);
int node_remove_proxy_dependency(Node *node, const char *unit_name);

int node_create_request(
                AgentRequest **ret,
                Node *node,
                const char *method,
                agent_request_response_t cb,
                void *userdata,
                free_func_t free_userdata);

void node_remove_proxy_monitor(Node *node, ProxyMonitor *proxy_monitor);

int node_method_get_unit_uint64_property_sync(Node *node, char *unit, char *property, uint64_t *value);
void node_enable_metrics(Node *node);
void node_disable_metrics(Node *node);

DEFINE_CLEANUP_FUNC(Node, node_unref)
#define _cleanup_node_ _cleanup_(node_unrefp)
DEFINE_CLEANUP_FUNC(AgentRequest, agent_request_unref)
#define _cleanup_agent_request_ _cleanup_(agent_request_unrefp)
