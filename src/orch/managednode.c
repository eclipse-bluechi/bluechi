#include "managednode.h"
#include "manager.h"


ManagedNode *managed_node_new(Manager *manager, sd_bus *bus) {
        ManagedNode *node = malloc0(sizeof(ManagedNode));

        if (node != NULL) {
                node->manager = manager;
                node->bus = sd_bus_ref(bus);
                LIST_INIT(nodes, node);
        }

        return node;
}

ManagedNode *managed_node_ref(ManagedNode *node) {
        node->ref_count++;
        return node;
}

void managed_node_unref(ManagedNode *node) {
        node->ref_count--;
        if (node->ref_count != 0) {
                return;
        }
        sd_bus_unref(node->bus);
        free(node->name);
        free(node);
}

void managed_node_unrefp(ManagedNode **nodep) {
        if (nodep && *nodep) {
                managed_node_unref(*nodep);
                *nodep = NULL;
        }
}
