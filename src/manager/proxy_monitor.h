/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "libhirte/common/common.h"

#include "monitor.h"
#include "types.h"

struct ProxyMonitor {
        int ref_count;

        Node *node; /* weak ref */
        Subscription *subscription;

        bool added_dep;

        char *unit_name;
        Node *target_node;

        char *proxy_object_path;

        unit_new_handler_func_t *handle_unit_new;
        unit_removed_handler_func_t *handle_unit_removed;
        unit_state_changed_handler_func_t *handle_unit_state_changed;
        unit_property_changed_handler_func_t *handle_unit_property_changed;

        LIST_FIELDS(ProxyMonitor, monitors);
};

Subscription *create_proxy_monitor_subscription(ProxyMonitor *monitor, const char *node, const char *unit);

ProxyMonitor *proxy_monitor_new(
                Node *node, const char *target_node_name, const char *unit_name, const char *proxy_object_path);
ProxyMonitor *proxy_monitor_ref(ProxyMonitor *monitor);
void proxy_monitor_unref(ProxyMonitor *monitor);

int proxy_monitor_set_target_node(ProxyMonitor *monitor, Node *target_node);

void proxy_monitor_close(ProxyMonitor *monitor);

int proxy_monitor_on_unit_new(void *userdata, const char *node, const char *unit, const char *reason);
int proxy_monitor_on_unit_state_changed(
                void *userdata,
                const char *node,
                const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason);
int proxy_monitor_on_unit_removed(void *userdata, const char *node, const char *unit, const char *reason);
int proxy_monitor_on_unit_property_changed(
                void *userdata, const char *node, const char *unit, const char *interface, sd_bus_message *m);

DEFINE_CLEANUP_FUNC(ProxyMonitor, proxy_monitor_unref)
#define _cleanup_proxy_monitor_ _cleanup_(proxy_monitor_unrefp)

void proxy_monitor_send_error(ProxyMonitor *monitor, const char *message);
int proxy_monitor_send_new(ProxyMonitor *monitor, const char *reason);
int proxy_monitor_send_removed(ProxyMonitor *monitor, const char *reason);
int proxy_monitor_send_state_changed(
                ProxyMonitor *monitor, const char *active_state, const char *substate, const char *reason);
int proxy_monitor_start_dep_unit(ProxyMonitor *monitor);
int proxy_monitor_stop_dep_unit(ProxyMonitor *monitor);
