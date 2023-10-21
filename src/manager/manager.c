/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <string.h>
#include <systemd/sd-daemon.h>

#include "libbluechi/bus/bus.h"
#include "libbluechi/bus/utils.h"
#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/log/log.h"
#include "libbluechi/service/shutdown.h"
#include "libbluechi/socket.h"

#include "job.h"
#include "manager.h"
#include "metrics.h"
#include "monitor.h"
#include "node.h"

#define DEBUG_MESSAGES 0

Manager *manager_new(void) {
        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                bc_log_errorf("Failed to create event loop: %s", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = strdup(BC_DBUS_NAME);
        if (service_name == NULL) {
                bc_log_error("Out of memory");
                return NULL;
        }

        Manager *manager = malloc0(sizeof(Manager));
        if (manager != NULL) {
                manager->ref_count = 1;
                manager->api_bus_service_name = steal_pointer(&service_name);
                manager->event = steal_pointer(&event);
                manager->metrics_enabled = false;
                LIST_HEAD_INIT(manager->nodes);
                LIST_HEAD_INIT(manager->anonymous_nodes);
                LIST_HEAD_INIT(manager->jobs);
                LIST_HEAD_INIT(manager->monitors);
                LIST_HEAD_INIT(manager->all_subscriptions);
        }

        return manager;
}

Manager *manager_ref(Manager *manager) {
        manager->ref_count++;
        return manager;
}

void manager_unref(Manager *manager) {
        manager->ref_count--;
        if (manager->ref_count != 0) {
                return;
        }

        Job *job = NULL;
        Job *next_job = NULL;
        LIST_FOREACH_SAFE(jobs, job, next_job, manager->jobs) {
                job_unref(job);
        }

        Subscription *sub = NULL;
        Subscription *next_sub = NULL;
        LIST_FOREACH_SAFE(all_subscriptions, sub, next_sub, manager->all_subscriptions) {
                manager_remove_subscription(manager, sub);
        }

        Monitor *monitor = NULL;
        Monitor *next_monitor = NULL;
        LIST_FOREACH_SAFE(monitors, monitor, next_monitor, manager->monitors) {
                monitor_unref(monitor);
        }

        Node *node = NULL;
        Node *next_node = NULL;
        LIST_FOREACH_SAFE(nodes, node, next_node, manager->nodes) {
                node_unref(node);
        }
        LIST_FOREACH_SAFE(nodes, node, next_node, manager->anonymous_nodes) {
                node_unref(node);
        }

        if (manager->config) {
                cfg_dispose(manager->config);
                manager->config = NULL;
        }

        sd_event_unrefp(&manager->event);

        free(manager->api_bus_service_name);

        sd_event_source_unrefp(&manager->node_connection_source);

        sd_bus_slot_unrefp(&manager->name_owner_changed_slot);
        sd_bus_slot_unrefp(&manager->filter_slot);
        sd_bus_slot_unrefp(&manager->manager_slot);
        sd_bus_slot_unrefp(&manager->metrics_slot);
        sd_bus_unrefp(&manager->api_bus);

        free(manager);
}

void manager_add_subscription(Manager *manager, Subscription *sub) {
        Node *node = NULL;

        LIST_APPEND(all_subscriptions, manager->all_subscriptions, subscription_ref(sub));

        if (subscription_has_node_wildcard(sub)) {
                LIST_FOREACH(nodes, node, manager->nodes) {
                        node_subscribe(node, sub);
                }
                return;
        }

        node = manager_find_node(manager, sub->node);
        if (node) {
                node_subscribe(node, sub);
        } else {
                bc_log_errorf("Warning: Subscription to non-existing node %s", sub->node);
        }
}

void manager_remove_subscription(Manager *manager, Subscription *sub) {
        Node *node = NULL;

        if (subscription_has_node_wildcard(sub)) {
                LIST_FOREACH(nodes, node, manager->nodes) {
                        node_unsubscribe(node, sub);
                }
        } else {
                node = manager_find_node(manager, sub->node);
                if (node) {
                        node_unsubscribe(node, sub);
                }
        }

        LIST_REMOVE(all_subscriptions, manager->all_subscriptions, sub);
        subscription_unref(sub);
}

Node *manager_find_node(Manager *manager, const char *name) {
        Node *node = NULL;

        LIST_FOREACH(nodes, node, manager->nodes) {
                if (strcmp(node->name, name) == 0) {
                        return node;
                }
        }

        return NULL;
}


Node *manager_find_node_by_path(Manager *manager, const char *path) {
        Node *node = NULL;

        LIST_FOREACH(nodes, node, manager->nodes) {
                if (streq(node->object_path, path)) {
                        return node;
                }
        }

        return NULL;
}

void manager_remove_node(Manager *manager, Node *node) {
        if (node->name) {
                manager->n_nodes--;
                LIST_REMOVE(nodes, manager->nodes, node);
        } else {
                LIST_REMOVE(nodes, manager->anonymous_nodes, node);
        }
        node_unref(node);
}

bool manager_add_job(Manager *manager, Job *job) {
        if (!job_export(job)) {
                return false;
        }

        int r = sd_bus_emit_signal(
                        manager->api_bus,
                        BC_MANAGER_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        "JobNew",
                        "uo",
                        job->id,
                        job->object_path);
        if (r < 0) {
                bc_log_errorf("Failed to emit JobNew signal: %s", strerror(-r));
                return false;
        }

        LIST_APPEND(jobs, manager->jobs, job_ref(job));
        return true;
}

void manager_remove_job(Manager *manager, Job *job, const char *result) {
        int r = sd_bus_emit_signal(
                        manager->api_bus,
                        BC_MANAGER_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        "JobRemoved",
                        "uosss",
                        job->id,
                        job->object_path,
                        job->node->name,
                        job->unit,
                        result);
        if (r < 0) {
                bc_log_errorf("Warning: Failed to send JobRemoved event: %s", strerror(-r));
                /* We can't really return a failure here */
        }

        LIST_REMOVE(jobs, manager->jobs, job);
        if (manager->metrics_enabled && streq(job->type, "start")) {
                metrics_produce_job_report(job);
        }
        job_unref(job);
}

void manager_job_state_changed(Manager *manager, uint32_t job_id, const char *state) {
        JobState new_state = job_state_from_string(state);
        Job *job = NULL;
        LIST_FOREACH(jobs, job, manager->jobs) {
                if (job->id == job_id) {
                        job_set_state(job, new_state);
                        break;
                }
        }
}

void manager_finish_job(Manager *manager, uint32_t job_id, const char *result) {
        Job *job = NULL;
        LIST_FOREACH(jobs, job, manager->jobs) {
                if (job->id == job_id) {
                        if (manager->metrics_enabled) {
                                job->job_end_micros = get_time_micros();
                        }
                        manager_remove_job(manager, job, result);
                        break;
                }
        }
}

Node *manager_add_node(Manager *manager, const char *name) {
        _cleanup_node_ Node *node = node_new(manager, name);
        if (node == NULL) {
                return NULL;
        }

        if (name) {
                manager->n_nodes++;
                LIST_APPEND(nodes, manager->nodes, node);
        } else {
                LIST_APPEND(nodes, manager->anonymous_nodes, node);
        }

        return steal_pointer(&node);
}

bool manager_set_port(Manager *manager, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                bc_log_errorf("Invalid port format '%s'", port_s);
                return false;
        }
        manager->port = port;
        return true;
}

bool manager_parse_config(Manager *manager, const char *configfile) {
        int result = 0;

        result = cfg_initialize(&manager->config);
        if (result != 0) {
                fprintf(stderr, "Error initializing configuration: '%s'.\n", strerror(-result));
                return false;
        }

        result = cfg_manager_def_conf(manager->config);
        if (result != 0) {
                fprintf(stderr, "Failed to set default settings for manager: %s", strerror(-result));
                return false;
        }

        result = cfg_load_complete_configuration(
                        manager->config, CFG_BC_DEFAULT_CONFIG, CFG_ETC_BC_CONF, CFG_ETC_BC_CONF_DIR);
        if (result != 0) {
                return false;
        }

        if (configfile != NULL) {
                result = cfg_load_from_file(manager->config, configfile);
                if (result < 0) {
                        fprintf(stderr,
                                "Error loading configuration file '%s': '%s'.\n",
                                configfile,
                                strerror(-result));
                        return false;
                } else if (result > 0) {
                        fprintf(stderr, "Error parsing configuration file '%s' on line %d\n", configfile, result);
                        return false;
                }
        }

        // set logging configuration
        bc_log_init(manager->config);

        const char *port = NULL;
        port = cfg_get_value(manager->config, CFG_MANAGER_PORT);
        if (port) {
                if (!manager_set_port(manager, port)) {
                        return false;
                }
        }

        const char *expected_nodes = cfg_get_value(manager->config, CFG_ALLOWED_NODE_NAMES);
        if (expected_nodes) {
                char *saveptr = NULL;

                /* copy string of expected nodes since */
                _cleanup_free_ char *expected_nodes_cpy = NULL;
                copy_str(&expected_nodes_cpy, expected_nodes);

                char *name = strtok_r(expected_nodes_cpy, ",", &saveptr);
                while (name != NULL) {
                        if (manager_find_node(manager, name) == NULL) {
                                manager_add_node(manager, name);
                        }

                        name = strtok_r(NULL, ",", &saveptr);
                }
        }

        _cleanup_free_ const char *dumped_cfg = cfg_dump(manager->config);
        bc_log_debug_with_data("Final configuration used", "\n%s", dumped_cfg);

        /* TODO: Handle per-node-name option section */

        return true;
}

static int manager_accept_node_connection(
                UNUSED sd_event_source *source, int fd, UNUSED uint32_t revents, void *userdata) {
        Manager *manager = userdata;
        Node *node = NULL;
        _cleanup_fd_ int nfd = accept_tcp_connection_request(fd);
        if (nfd < 0) {
                bc_log_errorf("Failed to accept TCP connection request: %s", strerror(-nfd));
                return -1;
        }

        _cleanup_sd_bus_ sd_bus *dbus_server = peer_bus_open_server(
                        manager->event, "managed-node", BC_DBUS_NAME, steal_fd(&nfd));
        if (dbus_server == NULL) {
                return -1;
        }

        int r = bus_socket_set_no_delay(dbus_server);
        if (r < 0) {
                bc_log_warn("Failed to set NO_DELAY on socket");
        }
        r = bus_socket_set_keepalive(dbus_server);
        if (r < 0) {
                bc_log_warn("Failed to set KEEPALIVE on socket");
        }

        /* Add anonymous node */
        node = manager_add_node(manager, NULL);
        if (node == NULL) {
                return -1;
        }

        if (!node_set_agent_bus(node, dbus_server)) {
                manager_remove_node(manager, steal_pointer(&node));
                return -1;
        }

        return 0;
}

static bool manager_setup_node_connection_handler(Manager *manager) {
        int r = 0;
        int accept_fd = -1;
        _cleanup_fd_ int tcp_fd = -1;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        int n = sd_listen_fds(0);
        if (n > 1) {
                bc_log_errorf("Received too many file descriptors - %d", n);
                return false;
        }

        if (n == 1) {
                accept_fd = SD_LISTEN_FDS_START;
        } else {
                tcp_fd = create_tcp_socket(manager->port);
                if (tcp_fd < 0) {
                        bc_log_errorf("Failed to create TCP socket: %s", strerror(errno));
                        return false;
                }
                accept_fd = tcp_fd;
        }

        r = sd_event_add_io(
                        manager->event, &event_source, accept_fd, EPOLLIN, manager_accept_node_connection, manager);
        if (r < 0) {
                bc_log_errorf("Failed to add io event: %s", strerror(-r));
                return false;
        }
        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                bc_log_errorf("Failed to set io fd own: %s", strerror(-r));
                return false;
        }

        // sd_event_set_io_fd_own takes care of closing accept_fd
        steal_fd(&tcp_fd);

        (void) sd_event_source_set_description(event_source, "node-accept-socket");

        manager->node_connection_source = steal_pointer(&event_source);

        return true;
}

/************************************************************************
 ****** org.eclipse.bluechi.Manager.Ping ******************
 ************************************************************************/

/* This is a test method for now, it just returns what you passed */
static int manager_method_ping(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *arg = NULL;

        int r = sd_bus_message_read(m, "s", &arg);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }
        return sd_bus_reply_method_return(m, "s", arg);
}


/************************************************************************
 ************** org.eclipse.bluechi.Manager.ListUnits *****
 ************************************************************************/

typedef struct ListUnitsRequest {
        sd_bus_message *request_message;

        int n_done;
        int n_sub_req;
        struct {
                Node *node;
                sd_bus_message *m;
                AgentRequest *agent_req;
        } sub_req[0];
} ListUnitsRequest;

static void list_unit_request_free(ListUnitsRequest *req) {
        sd_bus_message_unref(req->request_message);

        for (int i = 0; i < req->n_sub_req; i++) {
                node_unrefp(&req->sub_req[i].node);
                sd_bus_message_unrefp(&req->sub_req[i].m);
                agent_request_unrefp(&req->sub_req[i].agent_req);
        }

        free(req);
}

static void list_unit_request_freep(ListUnitsRequest **reqp) {
        if (reqp && *reqp) {
                list_unit_request_free(*reqp);
                *reqp = NULL;
        }
}

#define _cleanup_list_unit_request_ _cleanup_(list_unit_request_freep)


static int manager_method_list_units_encode_reply(ListUnitsRequest *req, sd_bus_message *reply) {
        int r = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, NODE_AND_UNIT_INFO_STRUCT_TYPESTRING);
        if (r < 0) {
                return r;
        }

        for (int i = 0; i < req->n_sub_req; i++) {
                const char *node_name = req->sub_req[i].node->name;
                sd_bus_message *m = req->sub_req[i].m;
                if (m == NULL) {
                        continue;
                }

                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, UNIT_INFO_STRUCT_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                while (sd_bus_message_at_end(m, false) == 0) {
                        r = sd_bus_message_open_container(
                                        reply, SD_BUS_TYPE_STRUCT, NODE_AND_UNIT_INFO_TYPESTRING);
                        if (r < 0) {
                                return r;
                        }

                        r = sd_bus_message_append(reply, "s", node_name);
                        if (r < 0) {
                                return r;
                        }

                        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, UNIT_INFO_TYPESTRING);
                        if (r < 0) {
                                return r;
                        }

                        r = sd_bus_message_copy(reply, m, true);
                        if (r < 0) {
                                return r;
                        }

                        r = sd_bus_message_close_container(reply);
                        if (r < 0) {
                                return r;
                        }
                        r = sd_bus_message_exit_container(m);
                        if (r < 0) {
                                return r;
                        }
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        return r;
                }
        }

        r = sd_bus_message_close_container(reply);
        if (r < 0) {
                return r;
        }

        return 0;
}


static void manager_method_list_units_done(ListUnitsRequest *req) {
        /* All sub_req-requests are done, collect results and free when done */
        _cleanup_list_unit_request_ ListUnitsRequest *free_me = req;

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r >= 0) {
                r = manager_method_list_units_encode_reply(req, reply);
        }
        if (r < 0) {
                sd_bus_reply_method_errorf(
                                req->request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message: %s",
                                strerror(-r));
                return;
        }

        r = sd_bus_message_send(reply);
        if (r < 0) {
                bc_log_errorf("Failed to send reply message: %s", strerror(-r));
                return;
        }
}

static void manager_method_list_units_maybe_done(ListUnitsRequest *req) {
        if (req->n_done == req->n_sub_req) {
                manager_method_list_units_done(req);
        }
}

static int manager_list_units_callback(
                AgentRequest *agent_req, UNUSED sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        ListUnitsRequest *req = agent_req->userdata;
        int i = 0;

        for (i = 0; i < req->n_sub_req; i++) {
                if (req->sub_req[i].agent_req == agent_req) {
                        break;
                }
        }

        assert(i != req->n_sub_req); /* we should have found the sub_req request */

        req->sub_req[i].m = sd_bus_message_ref(m);
        req->n_done++;

        manager_method_list_units_maybe_done(req);

        return 0;
}

static int manager_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        ListUnitsRequest *req = NULL;
        Node *node = NULL;

        req = malloc0_array(sizeof(*req), sizeof(req->sub_req[0]), manager->n_nodes);
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "Out of memory");
        }
        req->request_message = sd_bus_message_ref(m);

        int i = 0;
        LIST_FOREACH(nodes, node, manager->nodes) {
                _cleanup_agent_request_ AgentRequest *agent_req = node_request_list_units(
                                node, manager_list_units_callback, req, NULL);
                if (agent_req) {
                        req->sub_req[i].agent_req = steal_pointer(&agent_req);
                        req->sub_req[i].node = node_ref(node);
                        req->n_sub_req++;
                        i++;
                }
        }

        manager_method_list_units_maybe_done(req);

        return 1;
}

/************************************************************************
 ***** org.eclipse.bluechi.Manager.ListNodes **************
 ************************************************************************/

static int manager_method_list_encode_node(sd_bus_message *reply, Node *node) {
        int r = sd_bus_message_open_container(reply, SD_BUS_TYPE_STRUCT, "sos");
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_append(reply, "sos", node->name, node->object_path, node_get_status(node));
        if (r < 0) {
                return r;
        }
        return sd_bus_message_close_container(reply);
}

static int manager_method_list_nodes(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        Node *node = NULL;

        int r = sd_bus_message_new_method_return(m, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message: %s",
                                strerror(-r));
        }

        r = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "(sos)");
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the reply message: %s",
                                strerror(-r));
        }

        LIST_FOREACH(nodes, node, manager->nodes) {
                r = manager_method_list_encode_node(reply, node);
                if (r < 0) {
                        return sd_bus_reply_method_errorf(
                                        reply, SD_BUS_ERROR_FAILED, "Failed to encode a node: %s", strerror(-r));
                }
        }

        r = sd_bus_message_close_container(reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply, SD_BUS_ERROR_FAILED, "Failed to close message: %s", strerror(-r));
        }

        return sd_bus_message_send(reply);
}

/************************************************************************
 **** org.eclipse.bluechi.Manager.GetNode *****************
 ************************************************************************/

static int manager_method_get_node(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        Node *node = NULL;
        const char *node_name = NULL;

        int r = sd_bus_message_read(m, "s", &node_name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INVALID_ARGS, "Invalid argument for the node name");
        }

        node = manager_find_node(manager, node_name);
        if (node == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_SERVICE_UNKNOWN, "Node not found");
        }

        r = sd_bus_message_new_method_return(m, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message: %s",
                                strerror(-r));
        }

        r = sd_bus_message_append(reply, "o", node->object_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_FAILED,
                                "Failed to append the object path of the node to the reply message: %s",
                                strerror(-r));
        }

        return sd_bus_message_send(reply);
}

/************************************************************************
 ***** org.eclipse.bluechi.Manager.CreateMonitor **********
 ************************************************************************/

static int manager_method_create_monitor(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;

        _cleanup_monitor_ Monitor *monitor = monitor_new(manager, sd_bus_message_get_sender(m));
        if (monitor == NULL) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Failed to create new monitor");
        }

        if (!monitor_export(monitor)) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Failed to export monitor");
        }

        int r = sd_bus_message_new_method_return(m, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message for the monitor: %s",
                                strerror(-r));
        }

        r = sd_bus_message_append(reply, "o", monitor->object_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_FAILED,
                                "Failed to append the object path of the monitor to the reply message: %s",
                                strerror(-r));
        }

        r = sd_bus_message_send(reply);
        if (r < 0) {
                return r;
        }

        /* We reported it to the client, now keep it alive and keep track of it */
        LIST_APPEND(monitors, manager->monitors, monitor_ref(monitor));
        return 1;
}

/************************************************************************
 ***** org.eclipse.bluechi.Manager.EnableMetrics **********
 ************************************************************************/
static int manager_method_metrics_enable(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        Node *node = NULL;
        int r = 0;
        if (manager->metrics_enabled) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INCONSISTENT_MESSAGE, "Metrics already enabled");
        }
        r = metrics_export(manager);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to register metrics service: %s", strerror(-r));
        }
        manager->metrics_enabled = true;
        LIST_FOREACH(nodes, node, manager->nodes) {
                node_enable_metrics(node);
        }
        bc_log_debug("Metrics enabled");
        return sd_bus_reply_method_return(m, "");
}

/************************************************************************
 ***** org.eclipse.bluechi.Manager.DisableMetrics *********
 ************************************************************************/
static int manager_method_metrics_disable(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        Node *node = NULL;
        if (!manager->metrics_enabled) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INCONSISTENT_MESSAGE, "Metrics already disabled");
        }
        sd_bus_slot_unrefp(&manager->metrics_slot);
        manager->metrics_slot = NULL;
        manager->metrics_enabled = false;
        LIST_FOREACH(nodes, node, manager->nodes) {
                node_disable_metrics(node);
        }
        bc_log_debug("Metrics disabled");
        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 *** org.eclipse.bluechi.Manager.SetLogLevel ***************
 *************************************************************************/

static int manager_method_set_log_level(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *level = NULL;

        int r = sd_bus_message_read(m, "s", &level);
        if (r < 0) {
                bc_log_errorf("Failed to read the parameter: %s", strerror(-r));
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to read the parameter: %s", strerror(-r));
        }
        LogLevel loglevel = string_to_log_level(level);
        if (loglevel == LOG_LEVEL_INVALID) {
                bc_log_errorf("Invalid input for log level: %s", loglevel);
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid input for log level");
        }
        bc_log_set_level(loglevel);
        bc_log_infof("Log level changed to %s", level);
        return sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable manager_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Ping", "s", "s", manager_method_ping, 0),
        SD_BUS_METHOD("ListUnits", "", NODE_AND_UNIT_INFO_STRUCT_ARRAY_TYPESTRING, manager_method_list_units, 0),
        SD_BUS_METHOD("ListNodes", "", "a(sos)", manager_method_list_nodes, 0),
        SD_BUS_METHOD("GetNode", "s", "o", manager_method_get_node, 0),
        SD_BUS_METHOD("CreateMonitor", "", "o", manager_method_create_monitor, 0),
        SD_BUS_METHOD("SetLogLevel", "s", "", manager_method_set_log_level, 0),
        SD_BUS_METHOD("EnableMetrics", "", "", manager_method_metrics_enable, 0),
        SD_BUS_METHOD("DisableMetrics", "", "", manager_method_metrics_disable, 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobNew", "uo", SD_BUS_PARAM(id) SD_BUS_PARAM(job), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "JobRemoved",
                        "uosss",
                        SD_BUS_PARAM(id) SD_BUS_PARAM(job) SD_BUS_PARAM(node) SD_BUS_PARAM(unit)
                                        SD_BUS_PARAM(result),
                        0),
        SD_BUS_VTABLE_END
};

static int manager_dbus_filter(UNUSED sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        const char *object_path = sd_bus_message_get_path(m);
        const char *iface = sd_bus_message_get_interface(m);

        if (DEBUG_MESSAGES) {
                bc_log_infof("Incoming public message: path: %s, iface: %s, member: %s, signature: '%s'",
                             object_path,
                             iface,
                             sd_bus_message_get_member(m),
                             sd_bus_message_get_signature(m, true));
        }

        if (iface != NULL && streq(iface, NODE_INTERFACE)) {
                Node *node = manager_find_node_by_path(manager, object_path);

                /* All Node interface objects fail if the node is offline */
                if (node && !node_has_agent(node)) {
                        return sd_bus_reply_method_errorf(m, BC_BUS_ERROR_OFFLINE, "Node is offline");
                }
        }

        return 0;
}

void manager_remove_monitor(Manager *manager, Monitor *monitor) {
        LIST_REMOVE(monitors, manager->monitors, monitor);
        monitor_unref(monitor);
}

static void manager_client_disconnected(Manager *manager, const char *client_id) {
        /* Free any monitors owned by the client */

        Monitor *monitor = NULL;
        Monitor *next_monitor = NULL;
        LIST_FOREACH_SAFE(monitors, monitor, next_monitor, manager->monitors) {
                if (streq(monitor->client, client_id)) {
                        monitor_close(monitor);
                        manager_remove_monitor(manager, monitor);
                }
        }
}

static int manager_name_owner_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Manager *manager = userdata;
        const char *name = NULL;
        const char *old_owner = NULL;
        const char *new_owner = NULL;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);
        if (r < 0) {
                return r;
        }

        if (*name == ':' && *new_owner == 0) {
                manager_client_disconnected(manager, name);
        }

        return 0;
}

bool manager_start(Manager *manager) {
        bc_log_infof("Starting bluechi %s", CONFIG_H_BC_VERSION);
        if (manager == NULL) {
                return false;
        }

#ifdef USE_USER_API_BUS
        manager->api_bus = user_bus_open(manager->event);
#else
        manager->api_bus = system_bus_open(manager->event);
#endif
        if (manager->api_bus == NULL) {
                bc_log_error("Failed to open api dbus");
                return false;
        }

        /* Export all known nodes */
        Node *node = NULL;
        LIST_FOREACH(nodes, node, manager->nodes) {
                if (!node_export(node)) {
                        return false;
                }
        }

        int r = sd_bus_request_name(
                        manager->api_bus, manager->api_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                bc_log_errorf("Failed to acquire service name on api dbus: %s", strerror(-r));
                return false;
        }

        r = sd_bus_add_filter(manager->api_bus, &manager->filter_slot, manager_dbus_filter, manager);
        if (r < 0) {
                bc_log_errorf("Failed to add manager filter: %s", strerror(-r));
                return false;
        }

        r = sd_bus_match_signal(
                        manager->api_bus,
                        &manager->name_owner_changed_slot,
                        "org.freedesktop.DBus",
                        "/org/freedesktop/DBus",
                        "org.freedesktop.DBus",
                        "NameOwnerChanged",
                        manager_name_owner_changed,
                        manager);
        if (r < 0) {
                bc_log_errorf("Failed to add nameloist filter: %s", strerror(-r));
                return false;
        }

        r = sd_bus_add_object_vtable(
                        manager->api_bus,
                        &manager->manager_slot,
                        BC_MANAGER_OBJECT_PATH,
                        MANAGER_INTERFACE,
                        manager_vtable,
                        manager);
        if (r < 0) {
                bc_log_errorf("Failed to add manager vtable: %s", strerror(-r));
                return false;
        }

        if (!manager_setup_node_connection_handler(manager)) {
                return false;
        }

        r = shutdown_service_register(manager->api_bus, manager->event);
        if (r < 0) {
                bc_log_errorf("Failed to register shutdown service: %s", strerror(-r));
                return false;
        }

        r = event_loop_add_shutdown_signals(manager->event);
        if (r < 0) {
                bc_log_errorf("Failed to add signals to manager event loop: %s", strerror(-r));
                return false;
        }

        r = sd_event_loop(manager->event);
        if (r < 0) {
                bc_log_errorf("Starting event loop failed: %s", strerror(-r));
                return false;
        }

        return true;
}
