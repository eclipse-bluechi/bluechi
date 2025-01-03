/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <string.h>
#include <systemd/sd-daemon.h>

#include "libbluechi/bus/bus.h"
#include "libbluechi/bus/utils.h"
#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"
#include "libbluechi/common/event-util.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/log/log.h"
#include "libbluechi/service/shutdown.h"

#include "controller.h"
#include "job.h"
#include "metrics.h"
#include "monitor.h"
#include "node.h"

#define DEBUG_MESSAGES 0

Controller *controller_new(void) {
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

        _cleanup_free_ SocketOptions *socket_opts = socket_options_new();
        if (socket_opts == NULL) {
                bc_log_error("Out of memory");
                return NULL;
        }

        Controller *controller = malloc0(sizeof(Controller));
        if (controller != NULL) {
                controller->ref_count = 1;
                controller->api_bus_service_name = steal_pointer(&service_name);
                controller->event = steal_pointer(&event);
                controller->metrics_enabled = false;
                controller->number_of_nodes = 0;
                controller->number_of_nodes_online = 0;
                controller->peer_socket_options = steal_pointer(&socket_opts);
                controller->node_connection_tcp_socket_source = NULL;
                controller->node_connection_uds_socket_source = NULL;
                controller->node_connection_systemd_socket_source = NULL;
                LIST_HEAD_INIT(controller->nodes);
                LIST_HEAD_INIT(controller->anonymous_nodes);
                LIST_HEAD_INIT(controller->jobs);
                LIST_HEAD_INIT(controller->monitors);
                LIST_HEAD_INIT(controller->all_subscriptions);
        }

        return controller;
}

void controller_unref(Controller *controller) {
        assert(controller->ref_count > 0);

        controller->ref_count--;
        if (controller->ref_count != 0) {
                return;
        }

        bc_log_debug("Finalizing controller");

        /* These are removed in controller_stop */
        assert(LIST_IS_EMPTY(controller->jobs));
        assert(LIST_IS_EMPTY(controller->all_subscriptions));
        assert(LIST_IS_EMPTY(controller->monitors));
        assert(LIST_IS_EMPTY(controller->nodes));
        assert(LIST_IS_EMPTY(controller->anonymous_nodes));

        if (controller->config) {
                cfg_dispose(controller->config);
                controller->config = NULL;
        }

        sd_event_unrefp(&controller->event);

        free_and_null(controller->api_bus_service_name);
        free_and_null(controller->peer_socket_options);

        if (controller->node_connection_tcp_socket_source != NULL) {
                sd_event_source_unrefp(&controller->node_connection_tcp_socket_source);
                controller->node_connection_tcp_socket_source = NULL;
        }
        if (controller->node_connection_uds_socket_source != NULL) {
                sd_event_source_unrefp(&controller->node_connection_uds_socket_source);
                controller->node_connection_uds_socket_source = NULL;

                /* Remove UDS socket for proper cleanup and not cause an address in use error */
                unlink(CONFIG_H_UDS_SOCKET_PATH);
        }
        if (controller->node_connection_systemd_socket_source != NULL) {
                sd_event_source_unrefp(&controller->node_connection_systemd_socket_source);
                controller->node_connection_systemd_socket_source = NULL;

                /* Remove UDS socket for proper cleanup and not cause an address in use error even
                 * though the systemd socket might have been changed to listen on different socket
                 */
                unlink(CONFIG_H_UDS_SOCKET_PATH);
        }

        sd_bus_slot_unrefp(&controller->name_owner_changed_slot);
        sd_bus_slot_unrefp(&controller->filter_slot);
        sd_bus_slot_unrefp(&controller->controller_slot);
        sd_bus_slot_unrefp(&controller->metrics_slot);
        sd_bus_unrefp(&controller->api_bus);

        free(controller);
}

void controller_add_subscription(Controller *controller, Subscription *sub) {
        Node *node = NULL;

        LIST_APPEND(all_subscriptions, controller->all_subscriptions, subscription_ref(sub));

        if (subscription_has_node_wildcard(sub)) {
                LIST_FOREACH(nodes, node, controller->nodes) {
                        node_subscribe(node, sub);
                }
                return;
        }

        node = controller_find_node(controller, sub->node);
        if (node) {
                node_subscribe(node, sub);
        } else {
                bc_log_errorf("Warning: Subscription to non-existing node %s", sub->node);
        }
}

void controller_remove_subscription(Controller *controller, Subscription *sub) {
        Node *node = NULL;

        if (subscription_has_node_wildcard(sub)) {
                LIST_FOREACH(nodes, node, controller->nodes) {
                        node_unsubscribe(node, sub);
                }
        } else {
                node = controller_find_node(controller, sub->node);
                if (node) {
                        node_unsubscribe(node, sub);
                }
        }

        LIST_REMOVE(all_subscriptions, controller->all_subscriptions, sub);
        subscription_unref(sub);
}

Node *controller_find_node(Controller *controller, const char *name) {
        Node *node = NULL;

        LIST_FOREACH(nodes, node, controller->nodes) {
                if (strcmp(node->name, name) == 0) {
                        return node;
                }
        }

        return NULL;
}


Node *controller_find_node_by_path(Controller *controller, const char *path) {
        Node *node = NULL;

        LIST_FOREACH(nodes, node, controller->nodes) {
                if (streq(node->object_path, path)) {
                        return node;
                }
        }

        return NULL;
}

void controller_remove_node(Controller *controller, Node *node) {
        if (node->name) {
                controller->number_of_nodes--;
                LIST_REMOVE(nodes, controller->nodes, node);
        } else {
                LIST_REMOVE(nodes, controller->anonymous_nodes, node);
        }

        if (node_is_online(node)) {
                node_shutdown(node);
                controller->number_of_nodes_online--;
        }
        node_unref(node);
}

bool controller_add_job(Controller *controller, Job *job) {
        if (!job_export(job)) {
                return false;
        }

        int r = sd_bus_emit_signal(
                        controller->api_bus,
                        BC_CONTROLLER_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        "JobNew",
                        "uo",
                        job->id,
                        job->object_path);
        if (r < 0) {
                bc_log_errorf("Failed to emit JobNew signal: %s", strerror(-r));
                return false;
        }

        LIST_APPEND(jobs, controller->jobs, job_ref(job));
        return true;
}

void controller_remove_job(Controller *controller, Job *job, const char *result) {
        int r = sd_bus_emit_signal(
                        controller->api_bus,
                        BC_CONTROLLER_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
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

        LIST_REMOVE(jobs, controller->jobs, job);
        if (controller->metrics_enabled && streq(job->type, "start")) {
                metrics_produce_job_report(job);
        }
        job_unref(job);
}

void controller_job_state_changed(Controller *controller, uint32_t job_id, const char *state) {
        JobState new_state = job_state_from_string(state);
        Job *job = NULL;
        LIST_FOREACH(jobs, job, controller->jobs) {
                if (job->id == job_id) {
                        job_set_state(job, new_state);
                        break;
                }
        }
}

void controller_finish_job(Controller *controller, uint32_t job_id, const char *result) {
        Job *job = NULL;
        LIST_FOREACH(jobs, job, controller->jobs) {
                if (job->id == job_id) {
                        if (controller->metrics_enabled) {
                                job->job_end_micros = get_time_micros();
                        }
                        controller_remove_job(controller, job, result);
                        break;
                }
        }
}

Node *controller_add_node(Controller *controller, const char *name) {
        _cleanup_node_ Node *node = node_new(controller, name);
        if (node == NULL) {
                return NULL;
        }

        if (name) {
                controller->number_of_nodes++;
                LIST_APPEND(nodes, controller->nodes, node);
        } else {
                LIST_APPEND(nodes, controller->anonymous_nodes, node);
        }

        return steal_pointer(&node);
}

bool controller_set_use_tcp(Controller *controller, bool use_tcp) {
        controller->use_tcp = use_tcp;
        return true;
}

bool controller_set_use_uds(Controller *controller, bool use_uds) {
        controller->use_uds = use_uds;
        return true;
}


bool controller_set_port(Controller *controller, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                bc_log_errorf("Invalid port format '%s'", port_s);
                return false;
        }
        controller->port = port;
        return true;
}

bool controller_set_heartbeat_interval(Controller *controller, const char *interval_msec) {
        long interval = 0;

        if (!parse_long(interval_msec, &interval)) {
                bc_log_errorf("Invalid heartbeat interval format '%s'", interval_msec);
                return false;
        }
        controller->heartbeat_interval_msec = interval;
        return true;
}

bool controller_set_heartbeat_threshold(Controller *controller, const char *threshold_msec) {
        long threshold = 0;

        if (!parse_long(threshold_msec, &threshold)) {
                bc_log_errorf("Invalid heartbeat threshold format '%s'", threshold_msec);
                return false;
        }
        controller->heartbeat_threshold_msec = threshold;
        return true;
}

bool controller_parse_config(Controller *controller, const char *configfile) {
        int result = 0;

        result = cfg_initialize(&controller->config);
        if (result != 0) {
                fprintf(stderr, "Error initializing configuration: '%s'.\n", strerror(-result));
                return false;
        }

        result = cfg_controller_def_conf(controller->config);
        if (result != 0) {
                fprintf(stderr, "Failed to set default settings for controller: %s", strerror(-result));
                return false;
        }

        result = cfg_load_complete_configuration(
                        controller->config,
                        CFG_BC_DEFAULT_CONFIG,
                        CFG_ETC_BC_CONF,
                        CFG_ETC_BC_CONF_DIR,
                        configfile);
        if (result != 0) {
                return false;
        }
        return true;
}

bool controller_apply_config(Controller *controller) {
        if (!controller_set_use_tcp(
                            controller, cfg_get_bool_value(controller->config, CFG_CONTROLLER_USE_TCP))) {
                bc_log_error("Failed to set USE TCP");
                return false;
        }
        if (!controller_set_use_uds(
                            controller, cfg_get_bool_value(controller->config, CFG_CONTROLLER_USE_UDS))) {
                bc_log_error("Failed to set USE UDS");
                return false;
        }

        const char *port = NULL;
        port = cfg_get_value(controller->config, CFG_CONTROLLER_PORT);
        if (port) {
                if (!controller_set_port(controller, port)) {
                        return false;
                }
        }

        const char *expected_nodes = cfg_get_value(controller->config, CFG_ALLOWED_NODE_NAMES);
        if (expected_nodes) {
                char *saveptr = NULL;

                /* copy string of expected nodes since */
                _cleanup_free_ char *expected_nodes_cpy = NULL;
                copy_str(&expected_nodes_cpy, expected_nodes);

                char *name = strtok_r(expected_nodes_cpy, ",", &saveptr);
                while (name != NULL) {
                        if (controller_find_node(controller, name) == NULL) {
                                controller_add_node(controller, name);
                        }

                        name = strtok_r(NULL, ",", &saveptr);
                }
        }

        _cleanup_freev_ char **sections = cfg_list_sections(controller->config);
        if (sections == NULL) {
                bc_log_error("Failed to list config sections");
                return false;
        }
        for (size_t i = 0; sections[i] != NULL; i++) {
                const char *section = sections[i];
                if (!str_has_prefix(section, CFG_SECT_NODE_PREFIX)) {
                        continue;
                }
                const char *node_name = section + strlen(CFG_SECT_NODE_PREFIX);
                Node *node = controller_find_node(controller, node_name);
                if (node == NULL) {
                        /* Add it to the list if it is marked allowed */
                        bool allowed = cfg_s_get_bool_value(controller->config, section, CFG_ALLOWED);
                        if (!allowed) {
                                continue;
                        }
                        node = controller_add_node(controller, node_name);
                }

                const char *selinux_context = cfg_s_get_value(
                                controller->config, section, CFG_REQUIRED_SELINUX_CONTEXT);
                if (selinux_context && !node_set_required_selinux_context(node, selinux_context)) {
                        return false;
                }
        }

        const char *interval_msec = cfg_get_value(controller->config, CFG_HEARTBEAT_INTERVAL);
        if (interval_msec) {
                if (!controller_set_heartbeat_interval(controller, interval_msec)) {
                        return false;
                }
        }

        const char *threshold_msec = cfg_get_value(controller->config, CFG_NODE_HEARTBEAT_THRESHOLD);
        if (threshold_msec) {
                if (!controller_set_heartbeat_threshold(controller, threshold_msec)) {
                        return false;
                }
        }

        /* Set socket options used for peer connections with the agents */
        const char *keepidle = cfg_get_value(controller->config, CFG_TCP_KEEPALIVE_TIME);
        if (keepidle) {
                if (socket_options_set_tcp_keepidle(controller->peer_socket_options, keepidle) < 0) {
                        bc_log_error("Failed to set TCP KEEPIDLE");
                        return false;
                }
        }
        const char *keepintvl = cfg_get_value(controller->config, CFG_TCP_KEEPALIVE_INTERVAL);
        if (keepintvl) {
                if (socket_options_set_tcp_keepintvl(controller->peer_socket_options, keepintvl) < 0) {
                        bc_log_error("Failed to set TCP KEEPINTVL");
                        return false;
                }
        }
        const char *keepcnt = cfg_get_value(controller->config, CFG_TCP_KEEPALIVE_COUNT);
        if (keepcnt) {
                if (socket_options_set_tcp_keepcnt(controller->peer_socket_options, keepcnt) < 0) {
                        bc_log_error("Failed to set TCP KEEPCNT");
                        return false;
                }
        }
        if (socket_options_set_ip_recverr(
                            controller->peer_socket_options,
                            cfg_get_bool_value(controller->config, CFG_IP_RECEIVE_ERRORS)) < 0) {
                bc_log_error("Failed to set IP RECVERR");
                return false;
        }

        return true;
}

static int controller_accept_node_connection(
                UNUSED sd_event_source *source, int fd, UNUSED uint32_t revents, void *userdata) {
        Controller *controller = userdata;
        Node *node = NULL;
        _cleanup_fd_ int nfd = accept_connection_request(fd);
        if (nfd < 0) {
                bc_log_errorf("Failed to accept connection request: %s", strerror(-nfd));
                return -1;
        }

        _cleanup_sd_bus_ sd_bus *dbus_server = peer_bus_open_server(
                        controller->event, "managed-node", BC_DBUS_NAME, steal_fd(&nfd));
        if (dbus_server == NULL) {
                return -1;
        }

        bus_socket_set_options(dbus_server, controller->peer_socket_options);

        /* Add anonymous node */
        node = controller_add_node(controller, NULL);
        if (node == NULL) {
                return -1;
        }

        if (!node_set_agent_bus(node, dbus_server)) {
                controller_remove_node(controller, steal_pointer(&node));
                return -1;
        }

        return 0;
}

static bool controller_setup_systemd_socket_connection_handler(Controller *controller) {
        int r = 0;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        int n = sd_listen_fds(0);
        if (n < 1) {
                bc_log_debug("No socket unit file descriptor has been passed");
                return true;
        }
        if (n > 1) {
                bc_log_errorf("Received too many file descriptors from socket unit - %d", n);
                return false;
        }

        r = sd_event_add_io(
                        controller->event,
                        &event_source,
                        SD_LISTEN_FDS_START,
                        EPOLLIN,
                        controller_accept_node_connection,
                        controller);
        if (r < 0) {
                bc_log_errorf("Failed to add io event source for systemd socket unit: %s", strerror(-r));
                return false;
        }
        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                bc_log_errorf("Failed to set io fd own for systemd socket unit: %s", strerror(-r));
                return false;
        }

        (void) sd_event_source_set_description(event_source, "node-accept-systemd-socket");
        controller->node_connection_systemd_socket_source = steal_pointer(&event_source);

        bc_log_info("Waiting for connection requests on configured socket unit...");
        return true;
}

static bool controller_setup_tcp_connection_handler(Controller *controller) {
        int r = 0;
        _cleanup_fd_ int tcp_fd = -1;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        tcp_fd = create_tcp_socket(controller->port);
        if (tcp_fd < 0) {
                /*
                 * Check if the address is already in use and if the systemd file descriptor already uses it.
                 * In case both conditions are true, only log a warning and proceed as successful since a
                 * proper TCP socket incl. handler has already been set up.
                 */
                if (tcp_fd == -EADDRINUSE && controller->node_connection_systemd_socket_source != NULL &&
                    sd_is_socket_inet(SD_LISTEN_FDS_START, AF_UNSPEC, SOCK_STREAM, 1, controller->port) > 0) {
                        bc_log_warnf("TCP socket for port %d already setup with systemd socket unit",
                                     controller->port);
                        return true;
                }

                bc_log_errorf("Failed to create TCP socket: %s", strerror(-tcp_fd));
                return false;
        }

        r = sd_event_add_io(
                        controller->event,
                        &event_source,
                        tcp_fd,
                        EPOLLIN,
                        controller_accept_node_connection,
                        controller);
        if (r < 0) {
                bc_log_errorf("Failed to add io event for tcp socket: %s", strerror(-r));
                return false;
        }
        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                bc_log_errorf("Failed to set io fd own for tcp socket: %s", strerror(-r));
                return false;
        }
        // sd_event_set_io_fd_own takes care of closing tcp_fd
        steal_fd(&tcp_fd);

        (void) sd_event_source_set_description(event_source, "node-accept-tcp-socket");
        controller->node_connection_tcp_socket_source = steal_pointer(&event_source);

        bc_log_infof("Waiting for connection requests on port %d...", controller->port);
        return true;
}


static bool controller_setup_uds_connection_handler(Controller *controller) {
        int r = 0;
        _cleanup_fd_ int uds_fd = -1;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        uds_fd = create_uds_socket(CONFIG_H_UDS_SOCKET_PATH);
        if (uds_fd < 0) {
                /*
                 * Check if the uds path is already in use and if the systemd file descriptor already uses
                 * it. In case both conditions are true, only log a warning and proceed as successful since a
                 * proper UDS incl. handler has already been set up.
                 */
                if (uds_fd == -EADDRINUSE && controller->node_connection_systemd_socket_source != NULL &&
                    sd_is_socket_unix(SD_LISTEN_FDS_START, AF_UNIX, 1, CONFIG_H_UDS_SOCKET_PATH, 0) > 0) {
                        bc_log_warnf("UDS socket for path %s already setup with systemd socket unit",
                                     CONFIG_H_UDS_SOCKET_PATH);
                        return true;
                } else if (uds_fd == -EADDRINUSE) {
                        /* If address is in use, remove socket file and retry again */
                        unlink(CONFIG_H_UDS_SOCKET_PATH);
                        uds_fd = create_uds_socket(CONFIG_H_UDS_SOCKET_PATH);
                        if (uds_fd < 0) {
                                bc_log_errorf("Failed to create UDS socket: %s", strerror(-uds_fd));
                                return false;
                        }
                } else {
                        bc_log_errorf("Failed to create UDS socket: %s", strerror(-uds_fd));
                        return false;
                }
        }

        r = sd_event_add_io(
                        controller->event,
                        &event_source,
                        uds_fd,
                        EPOLLIN,
                        controller_accept_node_connection,
                        controller);
        if (r < 0) {
                bc_log_errorf("Failed to add io event for uds socket: %s", strerror(-r));
                return false;
        }
        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                bc_log_errorf("Failed to set io fd own for uds socket: %s", strerror(-r));
                return false;
        }
        // sd_event_set_io_fd_own takes care of closing uds_fd
        steal_fd(&uds_fd);

        (void) sd_event_source_set_description(event_source, "node-accept-uds-socket");
        controller->node_connection_uds_socket_source = steal_pointer(&event_source);

        bc_log_infof("Waiting for connection requests on socket %s...", CONFIG_H_UDS_SOCKET_PATH);
        return true;
}


static bool controller_setup_node_connection_handler(Controller *controller) {
        if (!controller_setup_systemd_socket_connection_handler(controller)) {
                return false;
        }
        if (controller->use_tcp && !controller_setup_tcp_connection_handler(controller)) {
                return false;
        }
        if (controller->use_uds && !controller_setup_uds_connection_handler(controller)) {
                return false;
        }

        if (controller->node_connection_systemd_socket_source == NULL &&
            controller->node_connection_tcp_socket_source == NULL &&
            controller->node_connection_uds_socket_source == NULL) {
                bc_log_error("No connection request handler configured");
                return false;
        }
        return true;
}

static int controller_reset_heartbeat_timer(Controller *controller, sd_event_source **event_source);

static bool controller_check_node_liveness(Controller *controller, Node *node, uint64_t now) {
        uint64_t diff = 0;

        if (controller->heartbeat_threshold_msec <= 0) {
                /* checking liveness of node by heartbeat disabled since configured threshold is <=0" */
                return true;
        }

        if (now == 0) {
                bc_log_error("Current time is wrong");
                return true;
        }

        if (now < node->last_seen_monotonic) {
                bc_log_error("Clock skew detected");
                return true;
        }

        diff = now - node->last_seen_monotonic;
        if (diff > (uint64_t) controller->heartbeat_threshold_msec * USEC_PER_MSEC) {
                bc_log_infof("Did not receive heartbeat from node '%s' since '%d'ms. Disconnecting it...",
                             node->name,
                             controller->heartbeat_threshold_msec);
                node_disconnect(node);
                return false;
        }

        return true;
}

static int controller_heartbeat_timer_callback(
                sd_event_source *event_source, UNUSED uint64_t usec, void *userdata) {
        Controller *controller = (Controller *) userdata;
        Node *node = NULL;
        uint64_t now = get_time_micros_monotonic();
        int r = 0;

        LIST_FOREACH(nodes, node, controller->nodes) {
                if (!node_is_online(node)) {
                        continue;
                }

                if (!controller_check_node_liveness(controller, node, now)) {
                        continue;
                }

                r = sd_bus_emit_signal(
                                node->agent_bus,
                                INTERNAL_CONTROLLER_OBJECT_PATH,
                                INTERNAL_CONTROLLER_INTERFACE,
                                "Heartbeat",
                                "");
                if (r < 0) {
                        bc_log_errorf("Failed to emit heartbeat signal to node '%s': %s",
                                      node->name,
                                      strerror(-r));
                }
        }

        r = controller_reset_heartbeat_timer(controller, &event_source);
        if (r < 0) {
                bc_log_errorf("Failed to reset controller heartbeat timer: %s", strerror(-r));
                return r;
        }

        return 0;
}

static int controller_reset_heartbeat_timer(Controller *controller, sd_event_source **event_source) {
        return event_reset_time_relative(
                        controller->event,
                        event_source,
                        CLOCK_BOOTTIME,
                        controller->heartbeat_interval_msec * USEC_PER_MSEC,
                        0,
                        controller_heartbeat_timer_callback,
                        controller,
                        0,
                        "controller-heartbeat-timer-source",
                        false);
}

static int controller_setup_heartbeat_timer(Controller *controller) {
        _cleanup_(sd_event_source_unrefp) sd_event_source *event_source = NULL;
        int r = 0;

        assert(controller);

        if (controller->heartbeat_interval_msec <= 0) {
                bc_log_warnf("Heartbeat disabled since configured interval '%d' is <=0",
                             controller->heartbeat_interval_msec);
                return 0;
        }

        r = controller_reset_heartbeat_timer(controller, &event_source);
        if (r < 0) {
                bc_log_errorf("Failed to reset controller heartbeat timer: %s", strerror(-r));
                return r;
        }

        return sd_event_source_set_floating(event_source, true);
}

/************************************************************************
 ***************** AgentFleetRequest ************************************
 ************************************************************************/

typedef struct AgentFleetRequest AgentFleetRequest;

typedef int (*agent_fleet_request_encode_reply_t)(AgentFleetRequest *req, sd_bus_message *reply);
typedef AgentRequest *(*agent_fleet_request_create_t)(
                Node *node, agent_request_response_t cb, void *userdata, free_func_t free_userdata);

typedef struct AgentFleetRequest {
        sd_bus_message *request_message;
        agent_fleet_request_encode_reply_t encode;

        int n_done;
        int n_sub_req;
        struct {
                Node *node;
                sd_bus_message *m;
                AgentRequest *agent_req;
        } sub_req[0];
} AgentFleetRequest;

static void agent_fleet_request_free(AgentFleetRequest *req) {
        sd_bus_message_unref(req->request_message);

        for (int i = 0; i < req->n_sub_req; i++) {
                node_unrefp(&req->sub_req[i].node);
                sd_bus_message_unrefp(&req->sub_req[i].m);
                agent_request_unrefp(&req->sub_req[i].agent_req);
        }

        free(req);
}

static void agent_fleet_request_freep(AgentFleetRequest **reqp) {
        if (reqp && *reqp) {
                agent_fleet_request_free(*reqp);
                *reqp = NULL;
        }
}

#define _cleanup_agent_fleet_request_ _cleanup_(agent_fleet_request_freep)

static void agent_fleet_request_done(AgentFleetRequest *req) {
        /* All sub_req-requests are done, collect results and free when done */
        UNUSED _cleanup_agent_fleet_request_ AgentFleetRequest *free_me = req;

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                sd_bus_reply_method_errorf(
                                req->request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message: %s",
                                strerror(-r));
                return;
        }

        r = req->encode(req, reply);
        if (r < 0) {
                sd_bus_reply_method_errorf(
                                req->request_message,
                                SD_BUS_ERROR_FAILED,
                                "Request to at least one node failed: %s",
                                strerror(-r));
                return;
        }

        r = sd_bus_message_send(reply);
        if (r < 0) {
                bc_log_errorf("Failed to send reply message: %s", strerror(-r));
                return;
        }
}

static void agent_fleet_request_maybe_done(AgentFleetRequest *req) {
        if (req->n_done == req->n_sub_req) {
                agent_fleet_request_done(req);
        }
}

static int agent_fleet_request_callback(
                AgentRequest *agent_req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        AgentFleetRequest *req = agent_req->userdata;
        int i = 0;

        for (i = 0; i < req->n_sub_req; i++) {
                if (req->sub_req[i].agent_req == agent_req) {
                        break;
                }
        }

        assert(i != req->n_sub_req); /* we should have found the sub_req request */

        req->sub_req[i].m = sd_bus_message_ref(m);
        req->n_done++;

        agent_fleet_request_maybe_done(req);

        return 0;
}

static int agent_fleet_request_start(
                sd_bus_message *request_message,
                Controller *controller,
                agent_fleet_request_create_t create_request,
                agent_fleet_request_encode_reply_t encode) {
        AgentFleetRequest *req = NULL;

        req = malloc0_array(sizeof(*req), sizeof(req->sub_req[0]), controller->number_of_nodes);
        if (req == NULL) {
                return sd_bus_reply_method_errorf(request_message, SD_BUS_ERROR_NO_MEMORY, "Out of memory");
        }
        req->request_message = sd_bus_message_ref(request_message);
        req->encode = encode;

        Node *node = NULL;
        int i = 0;
        LIST_FOREACH(nodes, node, controller->nodes) {
                _cleanup_agent_request_ AgentRequest *agent_req = create_request(
                                node, agent_fleet_request_callback, req, NULL);
                if (agent_req) {
                        req->sub_req[i].agent_req = steal_pointer(&agent_req);
                        req->sub_req[i].node = node_ref(node);
                        req->n_sub_req++;
                        i++;
                }
        }

// Disabling -Wanalyzer-malloc-leak temporarily due to false-positive
//      Leak detected is based on the assumption that controller_method_list_units_maybe_done is only
//      called once directly after iterating over the list - when the conditional to free req is false.
//      However, it does not take into account that controller_list_units_callback calls it for each node.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"

        agent_fleet_request_maybe_done(req);

        return 1;
}
#pragma GCC diagnostic pop

/************************************************************************
 ************** org.eclipse.bluechi.Controller.ListUnits *****
 ************************************************************************/

static int controller_method_list_units_encode_reply(AgentFleetRequest *req, sd_bus_message *reply) {
        int r = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, NODE_AND_UNIT_INFO_DICT_TYPESTRING);
        if (r < 0) {
                return r;
        }

        for (int i = 0; i < req->n_sub_req; i++) {
                const char *node_name = req->sub_req[i].node->name;
                sd_bus_message *m = req->sub_req[i].m;
                if (m == NULL) {
                        continue;
                }

                const sd_bus_error *err = sd_bus_message_get_error(m);
                if (err != NULL) {
                        return -sd_bus_message_get_errno(m);
                }

                r = sd_bus_message_open_container(reply, SD_BUS_TYPE_DICT_ENTRY, NODE_AND_UNIT_INFO_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                r = sd_bus_message_append(reply, "s", node_name);
                if (r < 0) {
                        return r;
                }

                r = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, UNIT_INFO_STRUCT_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, UNIT_INFO_STRUCT_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                while (sd_bus_message_at_end(m, false) == 0) {
                        r = sd_bus_message_copy(reply, m, true);
                        if (r < 0) {
                                return r;
                        }
                }

                r = sd_bus_message_close_container(reply);
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

        r = sd_bus_message_close_container(reply);
        if (r < 0) {
                return r;
        }

        return 0;
}

static int controller_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        return agent_fleet_request_start(
                        m, controller, node_request_list_units, controller_method_list_units_encode_reply);
}

/************************************************************************
 ***** org.eclipse.bluechi.Controller.ListUnitFiles **************
 ************************************************************************/

static int controller_method_list_unit_files_encode_reply(AgentFleetRequest *req, sd_bus_message *reply) {
        int r = sd_bus_message_open_container(
                        reply, SD_BUS_TYPE_ARRAY, NODE_AND_UNIT_FILE_INFO_DICT_TYPESTRING);
        if (r < 0) {
                return r;
        }

        for (int i = 0; i < req->n_sub_req; i++) {
                const char *node_name = req->sub_req[i].node->name;
                sd_bus_message *m = req->sub_req[i].m;
                if (m == NULL) {
                        continue;
                }

                const sd_bus_error *err = sd_bus_message_get_error(m);
                if (err != NULL) {
                        bc_log_errorf("Failed to list unit files for node '%s': %s", node_name, err->message);
                        return -sd_bus_message_get_errno(m);
                }

                r = sd_bus_message_open_container(
                                reply, SD_BUS_TYPE_DICT_ENTRY, NODE_AND_UNIT_FILE_INFO_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                r = sd_bus_message_append(reply, "s", node_name);
                if (r < 0) {
                        return r;
                }

                r = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, UNIT_FILE_INFO_STRUCT_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, UNIT_FILE_INFO_STRUCT_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                while (sd_bus_message_at_end(m, false) == 0) {
                        r = sd_bus_message_copy(reply, m, true);
                        if (r < 0) {
                                return r;
                        }
                }

                r = sd_bus_message_close_container(reply);
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

        r = sd_bus_message_close_container(reply);
        if (r < 0) {
                return r;
        }

        return 0;
}

static int controller_method_list_unit_files(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        return agent_fleet_request_start(
                        m,
                        controller,
                        node_request_list_unit_files,
                        controller_method_list_unit_files_encode_reply);
}

/************************************************************************
 ***** org.eclipse.bluechi.Controller.ListNodes **************
 ************************************************************************/

static int controller_method_list_encode_node(sd_bus_message *reply, Node *node) {
        int r = sd_bus_message_open_container(reply, SD_BUS_TYPE_STRUCT, "soss");
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_append(
                        reply, "soss", node->name, node->object_path, node_get_status(node), node->peer_ip);
        if (r < 0) {
                return r;
        }
        return sd_bus_message_close_container(reply);
}

static int controller_method_list_nodes(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
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

        r = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "(soss)");
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                reply,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the reply message: %s",
                                strerror(-r));
        }

        LIST_FOREACH(nodes, node, controller->nodes) {
                r = controller_method_list_encode_node(reply, node);
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
 **** org.eclipse.bluechi.Controller.GetNode *****************
 ************************************************************************/

static int controller_method_get_node(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        Node *node = NULL;
        const char *node_name = NULL;

        int r = sd_bus_message_read(m, "s", &node_name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INVALID_ARGS, "Invalid argument for the node name");
        }

        node = controller_find_node(controller, node_name);
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
 ***** org.eclipse.bluechi.Controller.CreateMonitor **********
 ************************************************************************/

static int controller_method_create_monitor(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;

        _cleanup_monitor_ Monitor *monitor = monitor_new(controller, sd_bus_message_get_sender(m));
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
        LIST_APPEND(monitors, controller->monitors, monitor_ref(monitor));
        return 1;
}

/************************************************************************
 ***** org.eclipse.bluechi.Controller.EnableMetrics **********
 ************************************************************************/
static int controller_method_metrics_enable(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        Node *node = NULL;
        int r = 0;
        if (controller->metrics_enabled) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INCONSISTENT_MESSAGE, "Metrics already enabled");
        }
        r = metrics_export(controller);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to register metrics service: %s", strerror(-r));
        }
        controller->metrics_enabled = true;
        LIST_FOREACH(nodes, node, controller->nodes) {
                node_enable_metrics(node);
        }
        bc_log_debug("Metrics enabled");
        return sd_bus_reply_method_return(m, "");
}

/************************************************************************
 ***** org.eclipse.bluechi.Controller.DisableMetrics *********
 ************************************************************************/
static int controller_method_metrics_disable(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        Node *node = NULL;
        if (!controller->metrics_enabled) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INCONSISTENT_MESSAGE, "Metrics already disabled");
        }
        sd_bus_slot_unrefp(&controller->metrics_slot);
        controller->metrics_slot = NULL;
        controller->metrics_enabled = false;
        LIST_FOREACH(nodes, node, controller->nodes) {
                node_disable_metrics(node);
        }
        bc_log_debug("Metrics disabled");
        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 *** org.eclipse.bluechi.Controller.SetLogLevel ***************
 *************************************************************************/

static int controller_method_set_log_level(
                sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *level = NULL;

        int r = sd_bus_message_read(m, "s", &level);
        if (r < 0) {
                bc_log_errorf("Failed to read the parameter: %s", strerror(-r));
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to read the parameter: %s", strerror(-r));
        }
        LogLevel loglevel = string_to_log_level(level);
        if (loglevel == LOG_LEVEL_INVALID) {
                bc_log_errorf("Invalid input for log level: %s", level);
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid input for log level");
        }
        bc_log_set_level(loglevel);
        bc_log_infof("Log level changed to %s", level);
        return sd_bus_reply_method_return(m, "");
}

/********************************************************
 **** org.eclipse.bluechi.Controller.Status ****************
 ********************************************************/

static char *controller_get_system_status(Controller *controller) {
        if (controller->number_of_nodes_online == 0) {
                return "down";
        } else if (controller->number_of_nodes_online == controller->number_of_nodes) {
                return "up";
        }
        return "degraded";
}

void controller_check_system_status(Controller *controller, int prev_number_of_nodes_online) {
        int diff = controller->number_of_nodes_online - prev_number_of_nodes_online;
        // clang-format off
        if ((prev_number_of_nodes_online == 0) ||                                          // at least one node online
                (prev_number_of_nodes_online == controller->number_of_nodes) ||            // at least one node offline
                ((prev_number_of_nodes_online + diff) == controller->number_of_nodes) ||   // all nodes online
                ((prev_number_of_nodes_online + diff) == 0)) {                             // all nodes offline
                // clang-format on
                int r = sd_bus_emit_properties_changed(
                                controller->api_bus,
                                BC_CONTROLLER_OBJECT_PATH,
                                CONTROLLER_INTERFACE,
                                "Status",
                                NULL);
                if (r < 0) {
                        bc_log_errorf("Failed to emit status property changed: %s", strerror(-r));
                }
        }
}

static int controller_property_get_status(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;

        return sd_bus_message_append(reply, "s", controller_get_system_status(controller));
}

static int controller_property_get_loglevel(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                UNUSED void *userdata,
                UNUSED sd_bus_error *ret_error) {
        const char *log_level = log_level_to_string(bc_log_get_level());
        return sd_bus_message_append(reply, "s", log_level);
}

static int controller_property_get_log_target(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                UNUSED void *userdata,
                UNUSED sd_bus_error *ret_error) {
        return sd_bus_message_append(reply, "s", log_target_to_str(bc_log_get_log_fn()));
}

static const sd_bus_vtable controller_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", NODE_AND_UNIT_INFO_DICT_ARRAY_TYPESTRING, controller_method_list_units, 0),
        SD_BUS_METHOD("ListUnitFiles",
                      "",
                      NODE_AND_UNIT_FILE_INFO_DICT_ARRAY_TYPESTRING,
                      controller_method_list_unit_files,
                      0),
        SD_BUS_METHOD("ListNodes", "", "a(soss)", controller_method_list_nodes, 0),
        SD_BUS_METHOD("GetNode", "s", "o", controller_method_get_node, 0),
        SD_BUS_METHOD("CreateMonitor", "", "o", controller_method_create_monitor, 0),
        SD_BUS_METHOD("SetLogLevel", "s", "", controller_method_set_log_level, 0),
        SD_BUS_METHOD("EnableMetrics", "", "", controller_method_metrics_enable, 0),
        SD_BUS_METHOD("DisableMetrics", "", "", controller_method_metrics_disable, 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobNew", "uo", SD_BUS_PARAM(id) SD_BUS_PARAM(job), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "JobRemoved",
                        "uosss",
                        SD_BUS_PARAM(id) SD_BUS_PARAM(job) SD_BUS_PARAM(node) SD_BUS_PARAM(unit)
                                        SD_BUS_PARAM(result),
                        0),
        SD_BUS_PROPERTY("LogLevel", "s", controller_property_get_loglevel, 0, SD_BUS_VTABLE_PROPERTY_EXPLICIT),
        SD_BUS_PROPERTY("LogTarget", "s", controller_property_get_log_target, 0, SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("Status", "s", controller_property_get_status, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
        SD_BUS_VTABLE_END
};

static int controller_dbus_filter(UNUSED sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
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
                Node *node = controller_find_node_by_path(controller, object_path);

                /* All Node interface objects fail if the node is offline */
                if (node && !node_has_agent(node)) {
                        return sd_bus_reply_method_errorf(m, BC_BUS_ERROR_OFFLINE, "Node is offline");
                }
        }

        return 0;
}

void controller_remove_monitor(Controller *controller, Monitor *monitor) {
        LIST_REMOVE(monitors, controller->monitors, monitor);
        monitor_unref(monitor);
}

static void controller_client_disconnected(Controller *controller, const char *client_id) {
        /* Free any monitors owned by the client */

        Monitor *monitor = NULL;
        Monitor *next_monitor = NULL;
        LIST_FOREACH_SAFE(monitors, monitor, next_monitor, controller->monitors) {
                if (streq(monitor->owner, client_id)) {
                        monitor_close(monitor);
                        controller_remove_monitor(controller, monitor);
                }
        }
}

static int controller_name_owner_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Controller *controller = userdata;
        const char *name = NULL;
        const char *old_owner = NULL;
        const char *new_owner = NULL;

        int r = sd_bus_message_read(m, "sss", &name, &old_owner, &new_owner);
        if (r < 0) {
                return r;
        }

        if (*name == ':' && *new_owner == 0) {
                controller_client_disconnected(controller, name);
        }

        return 0;
}

bool controller_start(Controller *controller) {
        bc_log_infof("Starting bluechi-controller %s", CONFIG_H_BC_VERSION);
        if (controller == NULL) {
                return false;
        }

#ifdef USE_USER_API_BUS
        controller->api_bus = user_bus_open(controller->event);
#else
        controller->api_bus = system_bus_open(controller->event);
#endif
        if (controller->api_bus == NULL) {
                bc_log_error("Failed to open api dbus");
                return false;
        }

        /* Export all known nodes */
        Node *node = NULL;
        LIST_FOREACH(nodes, node, controller->nodes) {
                if (!node_export(node)) {
                        return false;
                }
        }

        int r = sd_bus_request_name(
                        controller->api_bus, controller->api_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                bc_log_errorf("Failed to acquire service name on api dbus: %s", strerror(-r));
                return false;
        }

        r = sd_bus_add_filter(
                        controller->api_bus, &controller->filter_slot, controller_dbus_filter, controller);
        if (r < 0) {
                bc_log_errorf("Failed to add controller filter: %s", strerror(-r));
                return false;
        }

        r = sd_bus_match_signal(
                        controller->api_bus,
                        &controller->name_owner_changed_slot,
                        "org.freedesktop.DBus",
                        "/org/freedesktop/DBus",
                        "org.freedesktop.DBus",
                        "NameOwnerChanged",
                        controller_name_owner_changed,
                        controller);
        if (r < 0) {
                bc_log_errorf("Failed to add nameloist filter: %s", strerror(-r));
                return false;
        }

        r = sd_bus_add_object_vtable(
                        controller->api_bus,
                        &controller->controller_slot,
                        BC_CONTROLLER_OBJECT_PATH,
                        CONTROLLER_INTERFACE,
                        controller_vtable,
                        controller);
        if (r < 0) {
                bc_log_errorf("Failed to add controller vtable: %s", strerror(-r));
                return false;
        }

        if (!controller_setup_node_connection_handler(controller)) {
                return false;
        }

        r = controller_setup_heartbeat_timer(controller);
        if (r < 0) {
                bc_log_errorf("Failed to set up controller heartbeat timer: %s", strerror(-r));
                return false;
        }

        ShutdownHook hook;
        hook.shutdown = (ShutdownHookFn) controller_stop;
        hook.userdata = controller;
        r = event_loop_add_shutdown_signals(controller->event, &hook);
        if (r < 0) {
                bc_log_errorf("Failed to add signals to controller event loop: %s", strerror(-r));
                return false;
        }

        r = sd_event_loop(controller->event);
        if (r < 0) {
                bc_log_errorf("Starting event loop failed: %s", strerror(-r));
                return false;
        }

        return true;
}

void controller_stop(Controller *controller) {
        if (controller == NULL) {
                return;
        }

        bc_log_debug("Stopping controller");

        Job *job = NULL;
        Job *next_job = NULL;
        LIST_FOREACH_SAFE(jobs, job, next_job, controller->jobs) {
                controller_remove_job(controller, job, "cancelled due to shutdown");
        }

        Subscription *sub = NULL;
        Subscription *next_sub = NULL;
        LIST_FOREACH_SAFE(all_subscriptions, sub, next_sub, controller->all_subscriptions) {
                controller_remove_subscription(controller, sub);
        }

        Monitor *monitor = NULL;
        Monitor *next_monitor = NULL;
        LIST_FOREACH_SAFE(monitors, monitor, next_monitor, controller->monitors) {
                controller_remove_monitor(controller, monitor);
        }

        /* If all nodes were already offline, we don't need to emit a changed signal */
        bool status_changed = controller->number_of_nodes_online > 0;

        Node *node = NULL;
        Node *next_node = NULL;
        LIST_FOREACH_SAFE(nodes, node, next_node, controller->nodes) {
                controller_remove_node(controller, node);
        }
        LIST_FOREACH_SAFE(nodes, node, next_node, controller->anonymous_nodes) {
                controller_remove_node(controller, node);
        }

        /*
         * We won't handle any other events incl. node disconnected since we exit the event loop
         * right afterwards. Therefore, check the controller state and emit signal here.
         */
        if (status_changed) {
                controller_check_system_status(controller, controller->number_of_nodes_online);
        }
}
