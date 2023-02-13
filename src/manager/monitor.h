#pragma once

#include "libhirte/common/common.h"

#include "types.h"

struct Subscription {
        int ref_count;
        Monitor *monitor;

        char *node;
        char *unit;

        LIST_FIELDS(Subscription, subscriptions);     /* List in Monitor */
        LIST_FIELDS(Subscription, all_subscriptions); /* List in Manager */
};

Subscription *subscription_new(Monitor *monitor, const char *node, const char *unit);
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

int monitor_emit_unit_property_changed(Monitor *monitor, const char *node, const char *unit, sd_bus_message *m);
int monitor_emit_unit_new(Monitor *monitor, const char *node, const char *unit, const char *reason);
int monitor_emit_unit_removed(Monitor *monitor, const char *node, const char *unit, const char *reason);


DEFINE_CLEANUP_FUNC(Monitor, monitor_unref)
#define _cleanup_monitor_ _cleanup_(monitor_unrefp)
