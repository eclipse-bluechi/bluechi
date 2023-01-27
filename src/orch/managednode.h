#pragma once

#include "libhirte/common/common.h"

#include "types.h"

struct ManagedNode {
        int ref_count;

        Manager *manager; /* weak ref */

        sd_bus *bus;
        sd_bus_slot *bus_slot;

        LIST_FIELDS(ManagedNode, nodes);

        char *name;
        char *object_path;
};


ManagedNode *managed_node_new(Manager *manager);
ManagedNode *managed_node_ref(ManagedNode *node);
void managed_node_unref(ManagedNode *node);
void managed_node_unrefp(ManagedNode **nodep);

bool managed_node_set_agent_bus(ManagedNode *node, sd_bus *bus);

#define _cleanup_managed_node_ _cleanup_(managed_node_unrefp)
