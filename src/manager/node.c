#include "node.h"
#include "job.h"
#include "manager.h"

#define DEBUG_AGENT_MESSAGES 0

static int node_method_register(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int node_disconnected(sd_bus_message *message, void *userdata, sd_bus_error *error);
static int node_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_start_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_stop_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_restart_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_reload_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_property_get_nodename(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);
static int node_property_get_status(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);

static const sd_bus_vtable internal_manager_orchestrator_vtable[] = {
        SD_BUS_VTABLE_START(0), SD_BUS_METHOD("Register", "s", "", node_method_register, 0), SD_BUS_VTABLE_END
};

static const sd_bus_vtable node_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", "a(ssssssouso)", node_method_list_units, 0),
        SD_BUS_METHOD("StartUnit", "ss", "o", node_method_start_unit, 0),
        SD_BUS_METHOD("StopUnit", "ss", "o", node_method_stop_unit, 0),
        SD_BUS_METHOD("RestartUnit", "ss", "o", node_method_restart_unit, 0),
        SD_BUS_METHOD("ReloadUnit", "ss", "o", node_method_reload_unit, 0),
        SD_BUS_PROPERTY("Name", "s", node_property_get_nodename, 0, 0),
        SD_BUS_PROPERTY("Status", "s", node_property_get_status, 0, 0),
        SD_BUS_VTABLE_END
};

Node *node_new(Manager *manager, const char *name) {
        _cleanup_node_ Node *node = malloc0(sizeof(Node));
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

Node *node_ref(Node *node) {
        node->ref_count++;
        return node;
}

void node_unref(Node *node) {
        node->ref_count--;
        if (node->ref_count != 0) {
                return;
        }
        sd_bus_slot_unrefp(&node->export_slot);

        node_unset_agent_bus(node);

        free(node->name);
        free(node->object_path);
        free(node);
}

bool node_export(Node *node) {
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
        Node *node = userdata;
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


bool node_has_agent(Node *node) {
        return node->agent_bus != NULL;
}

static int node_match_job_done(UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        Manager *manager = node->manager;
        uint32_t hirte_job_id = 0;
        const char *result = NULL;

        int r = sd_bus_message_read(m, "us", &hirte_job_id, &result);
        if (r < 0) {
                fprintf(stderr, "Invalid JobDone signal\n");
                return 0;
        }

        manager_finish_job(manager, hirte_job_id, result);
        return 1;
}

bool node_set_agent_bus(Node *node, sd_bus *bus) {
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
                        node_unset_agent_bus(node);
                        fprintf(stderr, "Failed to add peer bus vtable: %s\n", strerror(-r));
                        return false;
                }
        } else {
                // Only listen to signals on named nodes
                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "JobDone",
                                node_match_job_done,
                                node);
                if (r < 0) {
                        fprintf(stderr, "Failed to add job-removed peer bus match: %s\n", strerror(-r));
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
                node_unset_agent_bus(node);
                fprintf(stderr, "Failed to request match for Disconnected message: %s\n", strerror(-r));
                return false;
        }

        if (DEBUG_AGENT_MESSAGES) {
                sd_bus_add_filter(bus, NULL, debug_messages_handler, node);
        }

        return true;
}

void node_unset_agent_bus(Node *node) {
        sd_bus_slot_unrefp(&node->disconnect_slot);
        node->disconnect_slot = NULL;

        sd_bus_slot_unrefp(&node->internal_manager_slot);
        node->internal_manager_slot = NULL;

        sd_bus_unrefp(&node->agent_bus);
        node->agent_bus = NULL;
}


/* org.containers.hirte.internal.Manager.Register(in s name)) */
static int node_method_register(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
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

        Node *named_node = manager_find_node(manager, name);
        if (named_node == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_SERVICE_UNKNOWN, "Unexpected node name");
        }

        if (node_has_agent(named_node)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_ADDRESS_IN_USE, "The node is already connected");
        }

        r = asprintf(&description, "node-%s", name);
        if (r >= 0) {
                (void) sd_bus_set_description(node->agent_bus, description);
        }

        /* Migrate the agent connection to the named node */
        _cleanup_sd_bus_ sd_bus *agent_bus = sd_bus_ref(node->agent_bus);
        if (!node_set_agent_bus(named_node, agent_bus)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Internal error: Couldn't set agent bus");
        }

        node_unset_agent_bus(node);

        printf("Registered managed node from fd %d as '%s'\n", sd_bus_get_fd(agent_bus), name);

        return sd_bus_reply_method_return(m, "");
}

static int node_disconnected(UNUSED sd_bus_message *message, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        Manager *manager = node->manager;

        if (node->name) {
                fprintf(stderr, "Node '%s' disconnected\n", node->name);
        } else {
                fprintf(stderr, "Anonymous node disconnected\n");
        }

        node_unset_agent_bus(node);

        /* Remove anoymous nodes when they disconnect */
        if (node->name == NULL) {
                manager_remove_node(manager, node);
        }

        return 0;
}

static int node_property_get_nodename(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;

        return sd_bus_message_append(reply, "s", node->name);
}

static int node_property_get_status(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        const char *status = NULL;

        if (node_has_agent(node)) {
                status = "online";
        } else {
                status = "offline";
        }

        return sd_bus_message_append(reply, "s", status);
}

AgentRequest *agent_request_ref(AgentRequest *req) {
        req->ref_count++;
        return req;
}

void agent_request_unref(AgentRequest *req) {
        Node *node = req->node;

        req->ref_count--;
        if (req->ref_count != 0) {
                return;
        }

        if (req->userdata && req->free_userdata) {
                req->free_userdata(req->userdata);
        }
        sd_bus_slot_unrefp(&req->slot);
        sd_bus_message_unrefp(&req->message);

        LIST_REMOVE(outstanding_requests, node->outstanding_requests, req);
        node_unref(req->node);
        free(req);
}

static AgentRequest *node_create_request(
                Node *node,
                const char *method,
                agent_request_response_t cb,
                void *userdata,
                free_func_t free_userdata) {
        _cleanup_agent_request_ AgentRequest *req = malloc0(sizeof(AgentRequest));
        if (req == NULL) {
                return NULL;
        }

        req->ref_count = 1;
        req->node = node_ref(node);
        LIST_INIT(outstanding_requests, req);
        req->userdata = userdata;
        req->cb = cb;
        req->free_userdata = free_userdata;

        LIST_APPEND(outstanding_requests, node->outstanding_requests, req);

        int r = sd_bus_message_new_method_call(
                        node->agent_bus,
                        &req->message,
                        HIRTE_AGENT_DBUS_NAME,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        method);
        if (r < 0) {
                return NULL;
        }

        return steal_pointer(&req);
}

static int agent_request_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_agent_request_ AgentRequest *req = userdata;

        return req->cb(req, m, ret_error);
}

static bool agent_request_start(AgentRequest *req) {
        Node *node = req->node;

        int r = sd_bus_call_async(
                        node->agent_bus,
                        &req->slot,
                        req->message,
                        agent_request_callback,
                        req,
                        HIRTE_DEFAULT_DBUS_TIMEOUT);
        if (r < 0) {
                fprintf(stderr, "Failed to call async: %s\n", strerror(-r));
                return false;
        }

        agent_request_ref(req); /* Keep alive while operation is outstanding */
        return true;
}

AgentRequest *node_request_list_units(
                Node *node, agent_request_response_t cb, void *userdata, free_func_t free_userdata) {
        if (!node_has_agent(node)) {
                return false;
        }

        _cleanup_agent_request_ AgentRequest *req = node_create_request(
                        node, "ListUnits", cb, userdata, free_userdata);
        if (req == NULL) {
                return false;
        }

        if (!agent_request_start(req)) {
                return false;
        }

        return steal_pointer(&req);
}

/*************************************************************************
 ********** org.containers.hirte.Node.ListUnits **************************
 ************************************************************************/

static int method_list_units_callback(AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        sd_bus_message *request_message = req->userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(request_message, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(request_message, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return sd_bus_reply_method_errorf(request_message, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return sd_bus_message_send(reply);
}

static int node_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;

        _cleanup_agent_request_ AgentRequest *agent_req = node_request_list_units(
                        node,
                        method_list_units_callback,
                        sd_bus_message_ref(m),
                        (free_func_t) sd_bus_message_unref);
        if (agent_req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/* Keep track of data related to setting up a job. For example calling
   the initial agent request before we know the job is actually going to
   happen. */
typedef struct {
        int ref_count;
        sd_bus_message *request_message;
        Job *job;
} JobSetup;

static JobSetup *job_setup_ref(JobSetup *setup) {
        setup->ref_count++;
        return setup;
}

static void job_setup_unref(JobSetup *setup) {
        setup->ref_count--;
        if (setup->ref_count != 0) {
                return;
        }

        job_unrefp(&setup->job);
        sd_bus_message_unrefp(&setup->request_message);
        free(setup);
}

DEFINE_CLEANUP_FUNC(JobSetup, job_setup_unref)
#define _cleanup_job_setup_ _cleanup_(job_setup_unrefp)

static JobSetup *job_setup_new(sd_bus_message *request_message, Node *node, const char *unit, const char *type) {
        _cleanup_job_setup_ JobSetup *setup = malloc0(sizeof(JobSetup));

        setup->ref_count = 1;
        setup->request_message = sd_bus_message_ref(request_message);
        setup->job = job_new(node, unit, type);
        if (setup->job == NULL) {
                NULL;
        }

        return steal_pointer(&setup);
}

static int unit_lifecycle_method_callback(AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        Node *node = req->node;
        Manager *manager = node->manager;
        JobSetup *setup = req->userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(setup->request_message, sd_bus_message_get_error(m));
        }

        if (!manager_add_job(manager, setup->job)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return sd_bus_reply_method_return(setup->request_message, "o", setup->job->object_path);
}

static int node_run_unit_lifecycle_method(
                sd_bus_message *m, Node *node, const char *job_type, const char *method) {
        _cleanup_agent_request_ AgentRequest *req = NULL;
        const char *unit = NULL;
        const char *mode = NULL;

        int r = sd_bus_message_read(m, "ss", &unit, &mode);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        _cleanup_job_setup_ JobSetup *setup = job_setup_new(m, node, unit, job_type);
        if (setup == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Out of memory");
        }

        req = node_create_request(
                        node,
                        method,
                        unit_lifecycle_method_callback,
                        job_setup_ref(setup),
                        (free_func_t) job_setup_unref);
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "ssu", unit, mode, setup->job->id);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!agent_request_start(req)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}


/*************************************************************************
 ********** org.containers.hirte.Node.StartUnit **************************
 ************************************************************************/

static int node_method_start_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "start", "StartUnit");
}

/*************************************************************************
 ********** org.containers.hirte.Node.StopUnit ***************************
 ************************************************************************/


static int node_method_stop_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "stop", "StopUnit");
}

/*************************************************************************
 ********** org.containers.hirte.Node.RestartUnit ************************
 ************************************************************************/

static int node_method_restart_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "restart", "RestartUnit");
}

/*************************************************************************
 ********** org.containers.hirte.Node.ReloadUnit **************************
 ************************************************************************/

static int node_method_reload_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "reload", "ReloadUnit");
}
