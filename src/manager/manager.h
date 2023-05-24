/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "libhirte/common/cfg.h"
#include "libhirte/common/common.h"

#include "types.h"

struct Manager {
        int ref_count;

        uint16_t port;
        char *api_bus_service_name;

        sd_event *event;
        sd_event_source *node_connection_source;

        sd_bus *api_bus;
        sd_bus_slot *manager_slot;
        sd_bus_slot *filter_slot;
        sd_bus_slot *name_owner_changed_slot;

        int n_nodes;
        LIST_HEAD(Node, nodes);
        LIST_HEAD(Node, anonymous_nodes);

        LIST_HEAD(Job, jobs);
        LIST_HEAD(Monitor, monitors);
        LIST_HEAD(Subscription, all_subscriptions);

        struct config *config;

        long debug_monitor_interval_msec;
};

Manager *manager_new(void);
Manager *manager_ref(Manager *manager);
void manager_unref(Manager *manager);

bool manager_set_port(Manager *manager, const char *port);
bool manager_parse_config(Manager *manager, const char *configfile);

bool manager_start(Manager *manager);
bool manager_stop(Manager *manager);

Node *manager_find_node(Manager *manager, const char *name);
Node *manager_find_node_by_path(Manager *manager, const char *path);
void manager_remove_node(Manager *manager, Node *node);

Node *manager_add_node(Manager *manager, const char *name);

bool manager_add_job(Manager *manager, Job *job);
void manager_remove_job(Manager *manager, Job *job, const char *result);
void manager_finish_job(Manager *manager, uint32_t job_id, const char *result);
void manager_job_state_changed(Manager *manager, uint32_t job_id, const char *state);

void manager_remove_monitor(Manager *manager, Monitor *monitor);

void manager_add_subscription(Manager *manager, Subscription *sub);
void manager_remove_subscription(Manager *manager, Subscription *sub);

DEFINE_CLEANUP_FUNC(Manager, manager_unref)
#define _cleanup_manager_ _cleanup_(manager_unrefp)
