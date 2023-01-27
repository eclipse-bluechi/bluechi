#include "managednode.h"
#include "manager.h"

#define DEBUG_AGENT_MESSAGES 0

static int managed_node_method_register(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int node_disconnected(sd_bus_message *message, void *userdata, sd_bus_error *error);
static int managed_node_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);

static const sd_bus_vtable internal_manager_orchestrator_vtable[] = {
        SD_BUS_VTABLE_START(0), SD_BUS_METHOD("Register", "s", "", managed_node_method_register, 0), SD_BUS_VTABLE_END
};

static const sd_bus_vtable node_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", "a(ssssssouso)", managed_node_method_list_units, 0),
        SD_BUS_VTABLE_END
};

ManagedNode *managed_node_new(Manager *manager, const char *name) {
        _cleanup_managed_node_ ManagedNode *node = malloc0(sizeof(ManagedNode));
        if (node == NULL) {
                return NULL;
        }

        node->ref_count = 1;
        node->manager = manager;
        LIST_INIT(nodes, node);

        if (name) {
                node->name = strdup(name);
                if (node->name == NULL) {
                        return NULL;
                }

                /* TODO: Should escape the name if needed */
                int r = asprintf(&node->object_path, "%s/%s", NODE_OBJECT_PATH_PREFIX, name);
                if (r < 0) {
                        return NULL;
                }
        }

        return steal_pointer(&node);
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
        if (node->export_slot) {
                sd_bus_slot_unref(node->export_slot);
        }

        managed_node_unset_agent_bus(node);

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

bool managed_node_export(ManagedNode *node) {
        Manager *manager = node->manager;

        assert(node->name != NULL);

        int r = sd_bus_add_object_vtable(
                        manager->user_dbus,
                        &node->export_slot,
                        node->object_path,
                        NODE_INTERFACE,
                        node_vtable,
                        node);
        if (r < 0) {
                fprintf(stderr, "Failed to add node vtable: %s\n", strerror(-r));
                return false;
        }

        return true;
}

static int debug_messages_handler(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        ManagedNode *node = userdata;
        if (node->name) {
                fprintf(stderr,
                        "Incomming message from node '%s' (fd %d): path: %s, iface: %s, member: %s, signature: '%s'\n",
                        node->name,
                        sd_bus_get_fd(node->agent_bus),
                        sd_bus_message_get_path(m),
                        sd_bus_message_get_interface(m),
                        sd_bus_message_get_member(m),
                        sd_bus_message_get_signature(m, true));
        } else {
                fprintf(stderr,
                        "Incomming message from node fd %d: path: %s, iface: %s, member: %s, signature: '%s'\n",
                        sd_bus_get_fd(node->agent_bus),
                        sd_bus_message_get_path(m),
                        sd_bus_message_get_interface(m),
                        sd_bus_message_get_member(m),
                        sd_bus_message_get_signature(m, true));
        }
        return 0;
}


bool managed_node_has_agent(ManagedNode *node) {
        return node->agent_bus != NULL;
}

bool managed_node_set_agent_bus(ManagedNode *node, sd_bus *bus) {
        int r = 0;

        if (node->agent_bus != NULL) {
                fprintf(stderr, "Error: Trying to add two agents for a node\n");
                return false;
        }

        node->agent_bus = sd_bus_ref(bus);
        if (node->name == NULL) {
                // We only connect to this on the unnamed nodes so register
                // can be called. We can't reconnect it during migration.
                r = sd_bus_add_object_vtable(
                                bus,
                                &node->internal_manager_slot,
                                INTERNAL_MANAGER_OBJECT_PATH,
                                INTERNAL_MANAGER_INTERFACE,
                                internal_manager_orchestrator_vtable,
                                node);
                if (r < 0) {
                        managed_node_unset_agent_bus(node);
                        fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                        return false;
                }
        }

        r = sd_bus_match_signal_async(
                        bus,
                        &node->disconnect_slot,
                        "org.freedesktop.DBus.Local",
                        "/org/freedesktop/DBus/Local",
                        "org.freedesktop.DBus.Local",
                        "Disconnected",
                        node_disconnected,
                        NULL,
                        node);
        if (r < 0) {
                managed_node_unset_agent_bus(node);
                fprintf(stderr, "Failed to request match for Disconnected message: %s\n", strerror(-r));
                return false;
        }

        if (DEBUG_AGENT_MESSAGES) {
                sd_bus_add_filter(bus, NULL, debug_messages_handler, node);
        }

        return true;
}

void managed_node_unset_agent_bus(ManagedNode *node) {
        if (node->disconnect_slot) {
                sd_bus_slot_unref(node->disconnect_slot);
                node->disconnect_slot = NULL;
        }

        if (node->internal_manager_slot) {
                sd_bus_slot_unref(node->internal_manager_slot);
                node->internal_manager_slot = NULL;
        }

        if (node->agent_bus) {
                sd_bus_unref(node->agent_bus);
                node->agent_bus = NULL;
        }
}


/* org.containers.hirte.internal.Manager.Register(in s name)) */
static int managed_node_method_register(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        ManagedNode *node = userdata;
        Manager *manager = node->manager;
        char *name = NULL;
        _cleanup_free_ char *description = NULL;

        /* Once we're not anonymous, don't allow register calls */
        if (node->name != NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_ADDRESS_IN_USE, "Can't register twice");
        }

        /* Read the parameters */
        int r = sd_bus_message_read(m, "s", &name);
        if (r < 0) {
                fprintf(stderr, "Failed to parse parameters: %s\n", strerror(-r));
                return r;
        }

        ManagedNode *named_node = manager_find_node(manager, name);
        if (named_node == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_SERVICE_UNKNOWN, "Unexpected node name");
        }

        if (managed_node_has_agent(named_node)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_ADDRESS_IN_USE, "The node is already connected");
        }

        r = asprintf(&description, "node-%s", name);
        if (r >= 0) {
                (void) sd_bus_set_description(node->agent_bus, description);
        }

        /* Migrate the agent connection to the named node */
        _cleanup_sd_bus_ sd_bus *agent_bus = sd_bus_ref(node->agent_bus);
        if (!managed_node_set_agent_bus(named_node, agent_bus)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Internal error: Couldn't set agent bus");
        }

        managed_node_unset_agent_bus(node);

        printf("Registered managed node from fd %d as '%s'\n", sd_bus_get_fd(agent_bus), name);

        return sd_bus_reply_method_return(m, "");
}

static int node_disconnected(UNUSED sd_bus_message *message, void *userdata, UNUSED sd_bus_error *error) {
        ManagedNode *node = userdata;
        Manager *manager = node->manager;

        if (node->name) {
                fprintf(stderr, "Node '%s' disconnected\n", node->name);
        } else {
                fprintf(stderr, "Anonymous node disconnected\n");
        }

        managed_node_unset_agent_bus(node);

        /* Remove anoymous nodes when they disconnect */
        if (node->name == NULL) {
                manager_remove_node(manager, node);
        }

        return 0;
}


ManagedRequest *managed_request_ref(ManagedRequest *req) {
        req->ref_count++;
        return req;
}

void managed_request_unref(ManagedRequest *req) {
        ManagedNode *node = req->node;

        req->ref_count--;
        if (req->ref_count != 0) {
                return;
        }

        if (req->request_message) {
                sd_bus_message_unref(req->request_message);
        }
        if (req->slot) {
                sd_bus_slot_unref(req->slot);
        }
        if (req->message) {
                sd_bus_message_unref(req->message);
        }

        LIST_REMOVE(outstanding_requests, node->outstanding_requests, req);
        managed_node_unref(req->node);
        free(req);
}

void managed_request_unrefp(ManagedRequest **reqp) {
        if (reqp && *reqp) {
                managed_request_unref(*reqp);
        }
}

static ManagedRequest *managed_node_create_request(
                ManagedNode *node, sd_bus_message *request_message, const char *method) {
        _cleanup_managed_request_ ManagedRequest *req = malloc0(sizeof(ManagedRequest));
        if (req == NULL) {
                return NULL;
        }

        req->ref_count = 1;
        req->node = managed_node_ref(node);
        LIST_INIT(outstanding_requests, req);
        req->request_message = sd_bus_message_ref(request_message);

        LIST_APPEND(outstanding_requests, node->outstanding_requests, req);

        int r = sd_bus_message_new_method_call(
                        node->agent_bus,
                        &req->message,
                        HIRTE_NODE_DBUS_NAME,
                        INTERNAL_NODE_OBJECT_PATH,
                        INTERNAL_NODE_INTERFACE,
                        method);
        if (r < 0) {
                return NULL;
        }


        return steal_pointer(&req);
}

static bool managed_request_start(ManagedRequest *req, sd_bus_message_handler_t callback) {
        ManagedNode *node = req->node;

        int r = sd_bus_call_async(
                        node->agent_bus, &req->slot, req->message, callback, req, HIRTE_DEFAULT_DBUS_TIMEOUT);
        if (r < 0) {
                fprintf(stderr, "Failed to call async: %s\n", strerror(-r));
                return false;
        }

        managed_request_ref(req);
        return true;
}

static int list_units_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_managed_request_ ManagedRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        sd_bus_message *reply = NULL;

        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int managed_node_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        ManagedNode *node = userdata;

        if (!managed_node_has_agent(node)) {
                return sd_bus_reply_method_errorf(m, HIRTE_BUS_ERROR_OFFLINE, "Node is offline");
        }

        _cleanup_managed_request_ ManagedRequest *req = managed_node_create_request(node, m, "ListUnits");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!managed_request_start(req, list_units_callback)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}
