/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libbluechi/log/log.h"

#include "controller.h"
#include "job.h"
#include "monitor.h"
#include "node.h"
#include "proxy_monitor.h"

Subscription *create_proxy_monitor_subscription(ProxyMonitor *monitor, const char *node) {
        Subscription *subscription = subscription_new(node);
        if (subscription == NULL) {
                return NULL;
        }

        subscription->monitor = monitor; /* Weak ref to avoid circular dep */
        subscription->free_monitor = NULL;

        subscription->handle_unit_new = proxy_monitor_on_unit_new;
        subscription->handle_unit_removed = proxy_monitor_on_unit_removed;
        subscription->handle_unit_state_changed = proxy_monitor_on_unit_state_changed;
        subscription->handle_unit_property_changed = proxy_monitor_on_unit_property_changed;

        return subscription;
}

ProxyMonitor *proxy_monitor_new(
                Node *node, const char *target_node_name, const char *unit_name, const char *proxy_object_path) {
        _cleanup_proxy_monitor_ ProxyMonitor *monitor = malloc0(sizeof(ProxyMonitor));
        if (monitor == NULL) {
                return NULL;
        }

        monitor->ref_count = 1;
        monitor->node = node;

        monitor->unit_name = strdup(unit_name);
        if (monitor->unit_name == NULL) {
                return NULL;
        }

        monitor->proxy_object_path = strdup(proxy_object_path);
        if (monitor->proxy_object_path == NULL) {
                return NULL;
        }

        monitor->subscription = create_proxy_monitor_subscription(monitor, target_node_name);
        if (monitor->subscription == NULL) {
                return NULL;
        }

        if (!subscription_add_unit(monitor->subscription, unit_name)) {
                return NULL;
        }

        return steal_pointer(&monitor);
}

ProxyMonitor *proxy_monitor_ref(ProxyMonitor *monitor) {
        monitor->ref_count++;
        return monitor;
}

static void proxy_monitor_drop_dep(ProxyMonitor *monitor);

void proxy_monitor_unref(ProxyMonitor *monitor) {
        monitor->ref_count--;
        if (monitor->ref_count != 0) {
                return;
        }

        if (monitor->target_node) {
                proxy_monitor_drop_dep(monitor);
                node_unref(monitor->target_node);
        }

        if (monitor->subscription) {
                subscription_unref(monitor->subscription);
        }

        free_and_null(monitor->unit_name);
        free_and_null(monitor->proxy_object_path);
        free(monitor);
}

int proxy_monitor_set_target_node(ProxyMonitor *monitor, Node *target_node) {
        monitor->target_node = node_ref(target_node);

        int res = node_add_proxy_dependency(target_node, monitor->unit_name);
        if (res < 0) {
                return res;
        }
        monitor->added_dep = true;
        return 0;
}

static void proxy_monitor_drop_dep(ProxyMonitor *monitor) {
        if (monitor->added_dep) {
                int res = node_remove_proxy_dependency(monitor->target_node, monitor->unit_name);
                if (res < 0) {
                        bc_log_error("Failed to remove proxy dependency");
                }
                monitor->added_dep = false;
        }
}

void proxy_monitor_close(ProxyMonitor *monitor) {
        proxy_monitor_drop_dep(monitor);
}

int proxy_monitor_on_unit_new(
                UNUSED void *userdata, UNUSED const char *node, UNUSED const char *unit, const char *reason) {
        ProxyMonitor *monitor = (ProxyMonitor *) userdata;
        bc_log_debugf("ProxyMonitor processing unit new %s %s %s", node, unit, reason);
        proxy_monitor_send_new(monitor, reason);
        return 0;
}

int proxy_monitor_on_unit_state_changed(
                UNUSED void *userdata,
                UNUSED const char *node,
                UNUSED const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason) {
        ProxyMonitor *monitor = (ProxyMonitor *) userdata;

        bc_log_debugf("ProxyMonitor processing unit state changed %s %s %s %s %s",
                      node,
                      unit,
                      active_state,
                      substate,
                      reason);
        proxy_monitor_send_state_changed(monitor, active_state, substate, reason);

        return 0;
}

int proxy_monitor_on_unit_removed(
                UNUSED void *userdata, UNUSED const char *node, UNUSED const char *unit, const char *reason) {
        ProxyMonitor *monitor = (ProxyMonitor *) userdata;
        bc_log_debugf("ProxyMonitor processing unit removed %s %s %s", node, unit, reason);
        proxy_monitor_send_removed(monitor, reason);
        return 0;
}

int proxy_monitor_on_unit_property_changed(
                UNUSED void *userdata,
                UNUSED const char *node,
                UNUSED const char *unit,
                UNUSED const char *interface,
                UNUSED sd_bus_message *m) {
        return 0;
}

static int proxy_monitor_send_error_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_monitor_ ProxyMonitor *monitor = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Failed to send proxy error on node %s with object path %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              sd_bus_message_get_error(m)->message);
        }
        return 0;
}

void proxy_monitor_send_error(ProxyMonitor *monitor, const char *message) {
        int r = sd_bus_call_method_async(
                        monitor->node->agent_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor->proxy_object_path,
                        INTERNAL_PROXY_INTERFACE,
                        "Error",
                        proxy_monitor_send_error_callback,
                        proxy_monitor_ref(monitor),
                        "s",
                        message);
        if (r < 0) {
                bc_log_errorf("Failed to call Error on node %s object %s with message '%s': %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              message,
                              strerror(-r));
        }
}

static int proxy_monitor_send_state_changed_callback(
                sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_monitor_ ProxyMonitor *monitor = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Failed to send proxy state change node %s with object path %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              sd_bus_message_get_error(m)->message);
        }
        return 0;
}

int proxy_monitor_send_state_changed(
                ProxyMonitor *monitor, const char *active_state, const char *substate, const char *reason) {
        int r = sd_bus_call_method_async(
                        monitor->node->agent_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor->proxy_object_path,
                        INTERNAL_PROXY_INTERFACE,
                        "TargetStateChanged",
                        proxy_monitor_send_state_changed_callback,
                        proxy_monitor_ref(monitor),
                        "sss",
                        active_state,
                        substate,
                        reason);
        if (r < 0) {
                bc_log_errorf("Failed to send proxy state changed on node %s object %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              strerror(-r));
        }
        return r;
}

static int proxy_monitor_send_new_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_monitor_ ProxyMonitor *monitor = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Failed to send proxy new %s with object path %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              sd_bus_message_get_error(m)->message);
        }
        return 0;
}

int proxy_monitor_send_new(ProxyMonitor *monitor, const char *reason) {
        int r = sd_bus_call_method_async(
                        monitor->node->agent_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor->proxy_object_path,
                        INTERNAL_PROXY_INTERFACE,
                        "TargetNew",
                        proxy_monitor_send_new_callback,
                        proxy_monitor_ref(monitor),
                        "s",
                        reason);
        if (r < 0) {
                bc_log_errorf("Failed to send proxy new on node %s object %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              strerror(-r));
        }
        return r;
}

static int proxy_monitor_send_removed_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_monitor_ ProxyMonitor *monitor = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Failed to send proxy removed %s with object path %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              sd_bus_message_get_error(m)->message);
        }
        return 0;
}

int proxy_monitor_send_removed(ProxyMonitor *monitor, const char *reason) {
        int r = sd_bus_call_method_async(
                        monitor->node->agent_bus,
                        NULL,
                        BC_INTERFACE_BASE_NAME,
                        monitor->proxy_object_path,
                        INTERNAL_PROXY_INTERFACE,
                        "TargetRemoved",
                        proxy_monitor_send_removed_callback,
                        proxy_monitor_ref(monitor),
                        "s",
                        reason);
        if (r < 0) {
                bc_log_errorf("Failed to send proxy removed on node %s object %s: %s",
                              monitor->node->name,
                              monitor->proxy_object_path,
                              strerror(-r));
        }
        return r;
}
