/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "libhirte/common/common.h"

#include "types.h"

typedef int(unit_property_changed_handler_func_t)(
                void *monitor, const char *node, const char *unit, const char *interface, sd_bus_message *m);
typedef int(unit_new_handler_func_t)(void *monitor, const char *node, const char *unit, const char *reason);
typedef int(unit_state_changed_handler_func_t)(
                void *monitor,
                const char *node,
                const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason);
typedef int(unit_removed_handler_func_t)(void *monitor, const char *node, const char *unit, const char *reason);


struct SubscribedUnit {
        char *name;
        LIST_FIELDS(SubscribedUnit, units);
};

struct Subscription {
        int ref_count;
        uint32_t id;

        void *monitor;
        free_func_t free_monitor;

        char *node;
        LIST_HEAD(SubscribedUnit, subscribed_units);

        LIST_FIELDS(Subscription, subscriptions);     /* List in Monitor */
        LIST_FIELDS(Subscription, all_subscriptions); /* List in Manager */

        unit_new_handler_func_t *handle_unit_new;
        unit_removed_handler_func_t *handle_unit_removed;
        unit_state_changed_handler_func_t *handle_unit_state_changed;
        unit_property_changed_handler_func_t *handle_unit_property_changed;
};

Subscription *subscription_new(const char *node);
Subscription *create_monitor_subscription(Monitor *monitor, const char *node);

bool subscription_add_unit(Subscription *sub, const char *unit);
bool subscription_has_node_wildcard(Subscription *sub);

Subscription *subscription_ref(Subscription *subscription);
void subscription_unref(Subscription *subscription);

DEFINE_CLEANUP_FUNC(Subscription, subscription_unref)
#define _cleanup_subscription_ _cleanup_(subscription_unrefp)

struct Monitor {
        int ref_count;
        uint32_t id;
        char *client;

        Manager *manager; /* weak ref */

        sd_bus_slot *export_slot;
        char *object_path;

        LIST_HEAD(Subscription, subscriptions);

        LIST_FIELDS(Monitor, monitors);
};

Monitor *monitor_new(Manager *manager, const char *client);
Monitor *monitor_ref(Monitor *monitor);
void monitor_unref(Monitor *monitor);

void monitor_close(Monitor *monitor);

bool monitor_export(Monitor *monitor);

int monitor_on_unit_property_changed(
                void *userdata, const char *node, const char *unit, const char *interface, sd_bus_message *m);
int monitor_on_unit_new(void *userdata, const char *node, const char *unit, const char *reason);
int monitor_on_unit_state_changed(
                void *userdata,
                const char *node,
                const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason);
int monitor_on_unit_removed(void *userdata, const char *node, const char *unit, const char *reason);

DEFINE_CLEANUP_FUNC(Monitor, monitor_unref)
#define _cleanup_monitor_ _cleanup_(monitor_unrefp)
