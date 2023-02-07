#pragma once

#include "libhirte/common/common.h"

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

        LIST_FIELDS(AgentRequest, outstanding_requests);
};

AgentRequest *agent_request_ref(AgentRequest *req);
void agent_request_unref(AgentRequest *req);

struct Node {
        int ref_count;

        Manager *manager; /* weak ref */

        /* public bus api */
        sd_bus_slot *export_slot;

        /* internal bus api */
        sd_bus *agent_bus;
        sd_bus_slot *internal_manager_slot;
        sd_bus_slot *disconnect_slot;

        LIST_FIELDS(Node, nodes);

        char *name; /* NULL for not yet unregistred nodes */
        char *object_path;

        LIST_HEAD(AgentRequest, outstanding_requests);
};


Node *node_new(Manager *manager, const char *name);
Node *node_ref(Node *node);
void node_unref(Node *node);

const char *node_get_status(Node *node);

bool node_export(Node *node);
bool node_has_agent(Node *node);
bool node_set_agent_bus(Node *node, sd_bus *bus);
void node_unset_agent_bus(Node *node);

AgentRequest *node_request_list_units(
                Node *node, agent_request_response_t cb, void *userdata, free_func_t free_userdata);

DEFINE_CLEANUP_FUNC(Node, node_unref)
#define _cleanup_node_ _cleanup_(node_unrefp)
DEFINE_CLEANUP_FUNC(AgentRequest, agent_request_unref)
#define _cleanup_agent_request_ _cleanup_(agent_request_unrefp)
