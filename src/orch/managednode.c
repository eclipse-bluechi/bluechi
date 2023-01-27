#include "managednode.h"
#include "manager.h"

static int managed_node_method_register(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int node_disconnected(sd_bus_message *message, void *userdata, sd_bus_error *error);

static const sd_bus_vtable internal_manager_orchestrator_vtable[] = {
        SD_BUS_VTABLE_START(0), SD_BUS_METHOD("Register", "s", "", managed_node_method_register, 0), SD_BUS_VTABLE_END
};

static const sd_bus_vtable node_vtable[] = { SD_BUS_VTABLE_START(0), SD_BUS_VTABLE_END };

ManagedNode *managed_node_new(Manager *manager) {
        _cleanup_managed_node_ ManagedNode *node = malloc0(sizeof(ManagedNode));

        if (node == NULL) {
                return NULL;
        }

        node->manager = manager;
        LIST_INIT(nodes, node);

        return steal_pointer(&node);
}

bool managed_node_set_agent_bus(ManagedNode *node, sd_bus *bus) {

        if (node->bus != NULL) {
                fprintf(stderr, "Error: Trying to add two agents for a node\n");
                return false;
        }

        node->bus = sd_bus_ref(bus);
        int r = sd_bus_add_object_vtable(
                        bus,
                        NULL,
                        HIRTE_INTERNAL_MANAGER_OBJECT_PATH,
                        INTERNAL_MANAGER_INTERFACE,
                        internal_manager_orchestrator_vtable,
                        node);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return false;
        }

        r = sd_bus_match_signal_async(
                        bus,
                        NULL,
                        "org.freedesktop.DBus.Local",
                        "/org/freedesktop/DBus/Local",
                        "org.freedesktop.DBus.Local",
                        "Disconnected",
                        node_disconnected,
                        NULL,
                        node);
        if (r < 0) {
                fprintf(stderr, "Failed to request match for Disconnected message: %s\n", strerror(-r));
                return false;
        }

        return true;
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
        if (node->bus_slot) {
                sd_bus_slot_unref(node->bus_slot);
        }
        if (node->bus) {
                sd_bus_unref(node->bus);
        }
        free(node->name);
        free(node->object_path);
        free(node);
}

void managed_node_unrefp(ManagedNode **nodep) {
        if (nodep && *nodep) {
                managed_node_unref(*nodep);
                *nodep = NULL;
        }
}

static int managed_node_method_register(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        ManagedNode *node = userdata;
        Manager *manager = node->manager;
        char *name = NULL;
        _cleanup_free_ char *description = NULL;

        /* Read the parameters */
        int r = sd_bus_message_read(m, "s", &name);
        if (r < 0) {
                fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
                return r;
        }

        if (node->name != NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_ADDRESS_IN_USE, "Can't register twice");
        }

        ManagedNode *existing = manager_find_node(manager, name);
        if (existing != NULL) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_ADDRESS_IN_USE, "Node name already registered");
        }

        node->name = strdup(name);
        if (node->name == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "No memory");
        }

        r = asprintf(&node->object_path, "%s/%s", NODE_OBJECT_PATH_PREFIX, name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "No memory");
        }

        r = asprintf(&description, "node-%s", name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "No memory");
        }

        (void) sd_bus_set_description(node->bus, description);

        r = sd_bus_add_object_vtable(
                        manager->user_dbus, &node->bus_slot, node->object_path, NODE_INTERFACE, node_vtable, node);
        if (r < 0) {
                fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        printf("Registered managed node from fd %d as '%s'\n", sd_bus_get_fd(node->bus), name);

        return sd_bus_reply_method_return(m, "");
}

static int node_disconnected(UNUSED sd_bus_message *message, void *userdata, UNUSED sd_bus_error *error) {
        ManagedNode *node = userdata;
        Manager *manager = node->manager;

        if (node->name) {
                printf("Node '%s' disconnected\n", node->name);
        } else {
                printf("Unregistered node disconnected\n");
        }

        if (node->bus_slot) {
                sd_bus_slot_unref(node->bus_slot);
                node->bus_slot = NULL;
        }

        if (node->bus) {
                sd_bus_close_unref(node->bus);
                node->bus = NULL;
        }

        manager_remove_node(manager, node);

        return 0;
}
