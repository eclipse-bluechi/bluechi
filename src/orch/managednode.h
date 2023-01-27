#pragma once

#include "libhirte/common/common.h"

#include "types.h"

struct ManagedRequest {
        int ref_count;
        ManagedNode *node;

        sd_bus_message *request_message;

        sd_bus_slot *slot;

        sd_bus_message *message;

        LIST_FIELDS(ManagedRequest, outstanding_requests);
};

ManagedRequest *managed_request_ref(ManagedRequest *req);
void managed_request_unref(ManagedRequest *req);
void managed_request_unrefp(ManagedRequest **req);


struct ManagedNode {
        int ref_count;

        Manager *manager; /* weak ref */

        /* public bus api */
        sd_bus_slot *export_slot;

        /* internal bus api */
        sd_bus *agent_bus;
        sd_bus_slot *internal_manager_slot;
        sd_bus_slot *disconnect_slot;

        LIST_FIELDS(ManagedNode, nodes);

        char *name; /* NULL for not yet unregistred nodes */
        char *object_path;

        LIST_HEAD(ManagedRequest, outstanding_requests);
};

ManagedNode *managed_node_new(Manager *manager, const char *name);
ManagedNode *managed_node_ref(ManagedNode *node);
void managed_node_unref(ManagedNode *node);
void managed_node_unrefp(ManagedNode **nodep);

bool managed_node_export(ManagedNode *node);
bool managed_node_has_agent(ManagedNode *node);
bool managed_node_set_agent_bus(ManagedNode *node, sd_bus *bus);
void managed_node_unset_agent_bus(ManagedNode *node);

#define _cleanup_managed_node_ _cleanup_(managed_node_unrefp)
#define _cleanup_managed_request_ _cleanup_(managed_request_unrefp)
