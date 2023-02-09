#include "monitor.h"
#include "job.h"
#include "manager.h"
#include "node.h"

Subscription *subscription_new(Monitor *monitor, const char *node, const char *unit) {
        _cleanup_subscription_ Subscription *subscription = malloc0(sizeof(Subscription));
        if (subscription == NULL) {
                return NULL;
        }

        subscription->ref_count = 1;
        subscription->monitor = monitor_ref(monitor);
        LIST_INIT(subscriptions, subscription);
        LIST_INIT(all_subscriptions, subscription);

        subscription->node = strdup(node);
        if (subscription->node == NULL) {
                return NULL;
        }

        subscription->unit = strdup(unit);
        if (subscription->unit == NULL) {
                return NULL;
        }

        return steal_pointer(&subscription);
}

Subscription *subscription_ref(Subscription *subscription) {
        subscription->ref_count++;
        return subscription;
}

void subscription_unref(Subscription *subscription) {
        subscription->ref_count--;
        if (subscription->ref_count != 0) {
                return;
        }

        monitor_unrefp(&subscription->monitor);
        free(subscription->unit);
        free(subscription->node);
        free(subscription);
}

static int monitor_method_close(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int monitor_method_subscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int monitor_method_unsubscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);

static const sd_bus_vtable monitor_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Subscribe", "ss", "", monitor_method_subscribe, 0),
        SD_BUS_METHOD("Unsubscribe", "ss", "", monitor_method_unsubscribe, 0),
        SD_BUS_METHOD("Close", "", "", monitor_method_close, 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitPropertiesChanged",
                        "ssa{sv}",
                        SD_BUS_PARAM(node) SD_BUS_PARAM(unit) SD_BUS_PARAM(properties),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitNew", "ss", SD_BUS_PARAM(node) SD_BUS_PARAM(unit), 0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitRemoved", "ss", SD_BUS_PARAM(node) SD_BUS_PARAM(unit), 0),
        SD_BUS_VTABLE_END
};

Monitor *monitor_new(Manager *manager, const char *client) {
        static uint32_t next_id = 0;
        _cleanup_monitor_ Monitor *monitor = malloc0(sizeof(Monitor));
        if (monitor == NULL) {
                return NULL;
        }

        monitor->ref_count = 1;
        monitor->manager = manager;
        LIST_INIT(monitors, monitor);
        monitor->id = ++next_id;
        LIST_HEAD_INIT(monitor->subscriptions);

        monitor->client = strdup(client);
        if (monitor->client == NULL) {
                return NULL;
        }

        int r = asprintf(&monitor->object_path, "%s/%u", MONITOR_OBJECT_PATH_PREFIX, monitor->id);
        if (r < 0) {
                return NULL;
        }

        return steal_pointer(&monitor);
}

Monitor *monitor_ref(Monitor *monitor) {
        monitor->ref_count++;
        return monitor;
}

void monitor_unref(Monitor *monitor) {
        monitor->ref_count--;
        if (monitor->ref_count != 0) {
                return;
        }

        sd_bus_slot_unrefp(&monitor->export_slot);
        free(monitor->object_path);
        free(monitor->client);
        free(monitor);
}

bool monitor_export(Monitor *monitor) {
        Manager *manager = monitor->manager;

        int r = sd_bus_add_object_vtable(
                        manager->user_dbus,
                        &monitor->export_slot,
                        monitor->object_path,
                        MONITOR_INTERFACE,
                        monitor_vtable,
                        monitor);
        if (r < 0) {
                fprintf(stderr, "Failed to add monitor vtable: %s\n", strerror(-r));
                return false;
        }

        return true;
}

void monitor_close(Monitor *monitor) {
        Manager *manager = monitor->manager;

        Subscription *sub = NULL;
        Subscription *next_sub = NULL;
        LIST_FOREACH_SAFE(subscriptions, sub, next_sub, monitor->subscriptions) {
                manager_remove_subscription(manager, sub);
                LIST_REMOVE(subscriptions, monitor->subscriptions, sub);
                subscription_unref(sub);
        }

        sd_bus_slot_unrefp(&monitor->export_slot);
        monitor->export_slot = NULL;
}

/*************************************************************************
 **************** org.containers.hirte.Monitor.Close *********************
 ************************************************************************/

static int monitor_method_close(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_monitor_ Monitor *monitor = monitor_ref(userdata);
        Manager *manager = monitor->manager;

        /* Ensure we don't close it twice somehow */
        if (monitor->export_slot == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        monitor_close(monitor);
        manager_remove_monitor(manager, monitor);

        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 **************** org.containers.hirte.Monitor.Subscribe *****************
 ************************************************************************/

static int monitor_method_subscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;
        Manager *manager = monitor->manager;
        const char *node = NULL;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "ss", &node, &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        _cleanup_subscription_ Subscription *sub = subscription_new(monitor, node, unit);
        if (sub == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        LIST_APPEND(subscriptions, monitor->subscriptions, subscription_ref(sub));

        manager_add_subscription(manager, sub);

        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 **************** org.containers.hirte.Monitor.Unsubscribe ***************
 ************************************************************************/

static Subscription *monitor_find_subscription(Monitor *monitor, const char *node, const char *unit) {
        Subscription *sub = NULL;

        LIST_FOREACH(subscriptions, sub, monitor->subscriptions) {
                if (streq(sub->node, node) && streq(sub->unit, unit)) {
                        return sub;
                }
        }

        return NULL;
}

static int monitor_method_unsubscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;
        Manager *manager = monitor->manager;
        const char *node = NULL;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "ss", &node, &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        Subscription *sub = monitor_find_subscription(monitor, node, unit);
        if (sub == NULL) {
                return sd_bus_reply_method_errorf(
                                m, HIRTE_BUS_ERROR_NO_SUCH_SUBSCRIPTION, "No such subscription");
        }

        manager_remove_subscription(manager, sub);
        LIST_REMOVE(subscriptions, monitor->subscriptions, sub);
        subscription_unref(sub);

        return sd_bus_reply_method_return(m, "");
}

int monitor_emit_unit_property_changed(Monitor *monitor, const char *node, const char *unit, sd_bus_message *m) {
        Manager *manager = monitor->manager;

        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        int r = sd_bus_message_new_signal(
                        manager->user_dbus,
                        &sig,
                        monitor->object_path,
                        MONITOR_INTERFACE,
                        "UnitPropertiesChanged");
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_append(sig, "ss", node, unit);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_skip(m, "s"); // Skip unit
        if (r < 0) {
                return r;
        }
        r = sd_bus_message_copy(sig, m, false);
        if (r < 0) {
                return r;
        }

        r = sd_bus_send_to(manager->user_dbus, sig, monitor->client, NULL);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_rewind(m, false);
}

int monitor_emit_unit_new(Monitor *monitor, const char *node, const char *unit) {
        Manager *manager = monitor->manager;

        return sd_bus_emit_signal(
                        manager->user_dbus, monitor->object_path, MONITOR_INTERFACE, "UnitNew", "ss", node, unit);
}

int monitor_emit_unit_removed(Monitor *monitor, const char *node, const char *unit) {
        Manager *manager = monitor->manager;

        return sd_bus_emit_signal(
                        manager->user_dbus,
                        monitor->object_path,
                        MONITOR_INTERFACE,
                        "UnitRemoved",
                        "ss",
                        node,
                        unit);
}
