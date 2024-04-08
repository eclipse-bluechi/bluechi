/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"
#include "libbluechi/socket.h"

#include "types.h"

struct Controller {
        int ref_count;

        uint16_t port;
        char *api_bus_service_name;
        long heartbeat_interval_msec;
        long heartbeat_threshold_msec;

        sd_event *event;
        sd_event_source *node_connection_source;

        sd_bus *api_bus;
        sd_bus_slot *controller_slot;
        sd_bus_slot *filter_slot;
        sd_bus_slot *name_owner_changed_slot;
        sd_bus_slot *metrics_slot;

        bool metrics_enabled;

        SocketOptions *peer_socket_options;

        int number_of_nodes;
        int number_of_nodes_online;
        LIST_HEAD(Node, nodes);
        LIST_HEAD(Node, anonymous_nodes);

        LIST_HEAD(Job, jobs);
        LIST_HEAD(Monitor, monitors);
        LIST_HEAD(Subscription, all_subscriptions);

        struct config *config;
};

Controller *controller_new(void);
void controller_unref(Controller *controller);

bool controller_set_port(Controller *controller, const char *port);
bool controller_parse_config(Controller *controller, const char *configfile);
bool controller_apply_config(Controller *controller);

bool controller_start(Controller *controller);
void controller_stop(Controller *controller);

void controller_check_system_status(Controller *controller, int prev_number_of_nodes_online);

Node *controller_find_node(Controller *controller, const char *name);
Node *controller_find_node_by_path(Controller *controller, const char *path);
void controller_remove_node(Controller *controller, Node *node);

Node *controller_add_node(Controller *controller, const char *name);

bool controller_add_job(Controller *controller, Job *job);
void controller_remove_job(Controller *controller, Job *job, const char *result);
void controller_finish_job(Controller *controller, uint32_t job_id, const char *result);
void controller_job_state_changed(Controller *controller, uint32_t job_id, const char *state);

void controller_remove_monitor(Controller *controller, Monitor *monitor);

void controller_add_subscription(Controller *controller, Subscription *sub);
void controller_remove_subscription(Controller *controller, Subscription *sub);

DEFINE_CLEANUP_FUNC(Controller, controller_unref)
#define _cleanup_controller_ _cleanup_(controller_unrefp)
