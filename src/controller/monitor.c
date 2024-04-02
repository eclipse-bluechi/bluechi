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

#include "libbluechi/bus/utils.h"

/****************************************
 ********** Subscription ****************
 ****************************************/

Subscription *subscription_new(const char *node) {
        static uint32_t next_id = 1;

        _cleanup_subscription_ Subscription *subscription = malloc0(sizeof(Subscription));
        if (subscription == NULL) {
                return NULL;
        }

        subscription->ref_count = 1;
        subscription->id = ++next_id;
        LIST_INIT(subscriptions, subscription);
        LIST_INIT(all_subscriptions, subscription);
        LIST_HEAD_INIT(subscription->subscribed_units);

        subscription->node = strdup(node);
        if (subscription->node == NULL) {
                return NULL;
        }

        return steal_pointer(&subscription);
}

Subscription *create_monitor_subscription(Monitor *monitor, const char *node) {
        Subscription *subscription = subscription_new(node);
        if (subscription == NULL) {
                return NULL;
        }

        subscription->monitor = monitor_ref(monitor);
        subscription->free_monitor = (free_func_t) monitor_unref;

        subscription->handle_unit_new = monitor_on_unit_new;
        subscription->handle_unit_removed = monitor_on_unit_removed;
        subscription->handle_unit_state_changed = monitor_on_unit_state_changed;
        subscription->handle_unit_property_changed = monitor_on_unit_property_changed;

        return subscription;
}

bool subscription_add_unit(Subscription *sub, const char *unit) {
        char *unit_copy = NULL;
        if (!copy_str(&unit_copy, unit)) {
                return false;
        }

        SubscribedUnit *su = malloc0(sizeof(SubscribedUnit));
        if (su == NULL) {
                free(unit_copy);
                return false;
        }
        su->name = unit_copy;
        LIST_APPEND(units, sub->subscribed_units, su);

        return true;
}

bool subscription_has_node_wildcard(Subscription *sub) {
        return sub->node != NULL && streq(sub->node, SYMBOL_WILDCARD);
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

        if (subscription->free_monitor) {
                subscription->free_monitor(subscription->monitor);
        }

        SubscribedUnit *su = NULL;
        SubscribedUnit *next_su = NULL;
        LIST_FOREACH_SAFE(units, su, next_su, subscription->subscribed_units) {
                LIST_REMOVE(units, subscription->subscribed_units, su);
                free_and_null(su->name);
                free_and_null(su);
        }

        free_and_null(subscription->node);
        free_and_null(subscription);
}


/***********************************
 ********** Monitor ****************
 ***********************************/

static int monitor_method_close(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int monitor_method_subscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int monitor_method_subscribe_list(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int monitor_method_unsubscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int monitor_method_add_peer(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int monitor_method_remove_peer(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);

static void monitor_emit_peer_removed(void *userdata, MonitorPeer *peer, const char *reason);

static const sd_bus_vtable monitor_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Subscribe", "ss", "u", monitor_method_subscribe, 0),
        SD_BUS_METHOD("SubscribeList", "sas", "u", monitor_method_subscribe_list, 0),
        SD_BUS_METHOD("Unsubscribe", "u", "", monitor_method_unsubscribe, 0),
        SD_BUS_METHOD("AddPeer", "s", "u", monitor_method_add_peer, 0),
        SD_BUS_METHOD("RemovePeer", "us", "", monitor_method_remove_peer, 0),
        SD_BUS_METHOD("Close", "", "", monitor_method_close, 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitPropertiesChanged",
                        "sssa{sv}",
                        SD_BUS_PARAM(node) SD_BUS_PARAM(unit) SD_BUS_PARAM(interface) SD_BUS_PARAM(properties),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitNew", "ss", SD_BUS_PARAM(node) SD_BUS_PARAM(unit), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitStateChanged",
                        "sssss",
                        SD_BUS_PARAM(node) SD_BUS_PARAM(unit) SD_BUS_PARAM(active_state)
                                        SD_BUS_PARAM(substate) SD_BUS_PARAM(reason),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitRemoved", "ss", SD_BUS_PARAM(node) SD_BUS_PARAM(unit), 0),
        SD_BUS_SIGNAL_WITH_NAMES("PeerRemoved", "s", SD_BUS_PARAM(reason), 0),
        SD_BUS_VTABLE_END
};

Monitor *monitor_new(Controller *controller, const char *owner) {
        static uint32_t next_id = 0;
        _cleanup_monitor_ Monitor *monitor = malloc0(sizeof(Monitor));
        if (monitor == NULL) {
                return NULL;
        }

        monitor->ref_count = 1;
        monitor->id = ++next_id;
        monitor->controller = controller;
        LIST_INIT(monitors, monitor);
        LIST_HEAD_INIT(monitor->subscriptions);
        LIST_HEAD_INIT(monitor->peers);

        monitor->owner = strdup(owner);
        if (monitor->owner == NULL) {
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
        free_and_null(monitor->object_path);
        free_and_null(monitor->owner);
        free_and_null(monitor);
}

bool monitor_export(Monitor *monitor) {
        Controller *controller = monitor->controller;

        int r = sd_bus_add_object_vtable(
                        controller->api_bus,
                        &monitor->export_slot,
                        monitor->object_path,
                        MONITOR_INTERFACE,
                        monitor_vtable,
                        monitor);
        if (r < 0) {
                bc_log_errorf("Failed to add monitor vtable: %s", strerror(-r));
                return false;
        }

        return true;
}

void monitor_close(Monitor *monitor) {
        Controller *controller = monitor->controller;

        Subscription *sub = NULL;
        Subscription *next_sub = NULL;
        LIST_FOREACH_SAFE(subscriptions, sub, next_sub, monitor->subscriptions) {
                controller_remove_subscription(controller, sub);
                LIST_REMOVE(subscriptions, monitor->subscriptions, sub);
                subscription_unref(sub);
        }

        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                monitor_emit_peer_removed(monitor, peer, "monitor closed");
                LIST_REMOVE(peers, monitor->peers, peer);
                free_and_null(peer->name);
                free_and_null(peer);
        }

        sd_bus_slot_unrefp(&monitor->export_slot);
        monitor->export_slot = NULL;
}

/***********************************************************
 *** org.eclipse.bluechi.Monitor.Close *********************
 ***********************************************************/

static int monitor_method_close(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_monitor_ Monitor *monitor = monitor_ref(userdata);
        Controller *controller = monitor->controller;

        /* Ensure we don't close it twice somehow */
        if (monitor->export_slot == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Monitor already closed");
        }

        monitor_close(monitor);
        controller_remove_monitor(controller, monitor);

        return sd_bus_reply_method_return(m, "");
}

/***********************************************************
 *** org.eclipse.bluechi.Monitor.Subscribe *****************
 ***********************************************************/

static int monitor_method_subscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;
        Controller *controller = monitor->controller;
        const char *node = NULL;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "ss", &node, &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the node or the unit: %s",
                                strerror(-r));
        }

        _cleanup_subscription_ Subscription *sub = create_monitor_subscription(monitor, node);
        if (sub == NULL) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a monitor subscription: %s",
                                strerror(-r));
        }

        if (!subscription_add_unit(sub, unit)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to add an unit to a subscription");
        }

        LIST_APPEND(subscriptions, monitor->subscriptions, subscription_ref(sub));
        controller_add_subscription(controller, sub);

        return sd_bus_reply_method_return(m, "u", sub->id);
}

/***********************************************************
 ***** org.eclipse.bluechi.Monitor.SubscribeList ***********
 ***********************************************************/

static int monitor_method_subscribe_list(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;
        Controller *controller = monitor->controller;

        const char *node = NULL;
        int r = sd_bus_message_read(m, "s", &node);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the node name: %s",
                                strerror(-r));
        }

        _cleanup_subscription_ Subscription *sub = create_monitor_subscription(monitor, node);
        if (sub == NULL) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a monitor subscription: %s",
                                strerror(-r));
        }

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "s");
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for an array of unit : %s",
                                strerror(-r));
        }

        while (sd_bus_message_at_end(m, false) == 0) {
                const char *unit = NULL;
                r = sd_bus_message_read(m, "s", &unit);
                if (r < 0) {
                        return sd_bus_reply_method_errorf(
                                        m,
                                        SD_BUS_ERROR_INVALID_ARGS,
                                        "Invalid arguments for units: %s",
                                        strerror(-r));
                }

                if (!subscription_add_unit(sub, unit)) {
                        return sd_bus_reply_method_errorf(
                                        m,
                                        SD_BUS_ERROR_FAILED,
                                        "Internal error when adding unit '%s' to subscription",
                                        unit);
                }
        }

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to exit message container while processing unit subscription: %s",
                                strerror(-r));
        }

        LIST_APPEND(subscriptions, monitor->subscriptions, subscription_ref(sub));
        controller_add_subscription(controller, sub);

        return sd_bus_reply_method_return(m, "u", sub->id);
}

/***********************************************************
 ********* org.eclipse.bluechi.Monitor.Unsubscribe *********
 ***********************************************************/

static Subscription *monitor_find_subscription(Monitor *monitor, uint32_t sub_id) {
        Subscription *sub = NULL;
        Subscription *next_sub = NULL;
        LIST_FOREACH_SAFE(subscriptions, sub, next_sub, monitor->subscriptions) {
                if (sub->id == sub_id) {
                        return sub;
                }
        }
        bc_log_debugf("Subscription ID '%d' for monitor '%s' not found", sub_id, monitor->object_path);

        return NULL;
}

static int monitor_method_unsubscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;
        Controller *controller = monitor->controller;
        uint32_t sub_id = 0;

        int r = sd_bus_message_read(m, "u", &sub_id);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid arguments for the subscription ID: %s",
                                strerror(-r));
        }

        Subscription *sub = monitor_find_subscription(monitor, sub_id);
        if (sub == NULL) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Subscription '%d' not found", sub_id);
        }

        controller_remove_subscription(controller, sub);
        LIST_REMOVE(subscriptions, monitor->subscriptions, sub);
        subscription_unref(sub);

        return sd_bus_reply_method_return(m, "");
}

/*******************************************************
 ********* org.eclipse.bluechi.Monitor.AddPeer *********
 *******************************************************/

static MonitorPeer *monitor_add_peer(Monitor *monitor, const char *peer_name) {
        static uint32_t next_peer_id = 0;

        MonitorPeer *peer = malloc0(sizeof(MonitorPeer));
        if (peer == NULL) {
                return NULL;
        }
        peer->name = strdup(peer_name);
        if (peer->name == NULL) {
                free(peer);
                return NULL;
        }
        peer->id = ++next_peer_id;

        LIST_APPEND(peers, monitor->peers, peer);

        bc_log_debugf("Added monitor peer '%s' to monitor '%s'", peer_name, monitor->object_path);
        return peer;
}

static bool monitor_has_peer_with_name(Monitor *monitor, const char *peer_name) {
        if (streq(monitor->owner, peer_name)) {
                return true;
        }

        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                if (streq(peer->name, peer_name)) {
                        return true;
                }
        }
        bc_log_debugf("Peer with name '%s' for monitor '%s' not found", peer_name, monitor->object_path);

        return false;
}

static int monitor_method_add_peer(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;

        const char *peer_name = NULL;

        int r = sd_bus_message_read(m, "s", &peer_name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the peer name: %s",
                                strerror(-r));
        }

        if (!bus_id_is_valid(peer_name)) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Peer name '%s' is not a valid bus name",
                                peer_name);
        }

        if (monitor_has_peer_with_name(monitor, peer_name)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_INVALID_ARGS, "Peer name '%s' has already been added", peer_name);
        }

        MonitorPeer *peer = monitor_add_peer(monitor, peer_name);
        if (peer == NULL) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_NO_MEMORY, "Failed to add peer '%s': OOM", peer_name);
        }

        return sd_bus_reply_method_return(m, "u", peer->id);
}

/*******************************************************
 ********* org.eclipse.bluechi.Monitor.RemovePeer *********
 *******************************************************/

static MonitorPeer *monitor_find_peer(Monitor *monitor, uint32_t peer_id) {
        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                if (peer->id == peer_id) {
                        return peer;
                }
        }
        bc_log_debugf("Peer with ID '%d' for monitor '%s' not found", peer_id, monitor->object_path);

        return NULL;
}

static int monitor_method_remove_peer(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Monitor *monitor = userdata;

        uint32_t peer_id = 0;
        const char *reason = NULL;

        int r = sd_bus_message_read(m, "us", &peer_id, &reason);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the peer name: %s",
                                strerror(-r));
        }

        MonitorPeer *peer = monitor_find_peer(monitor, peer_id);
        if (peer == NULL) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Peer with ID '%d' not found.", peer_id);
        }

        monitor_emit_peer_removed(monitor, peer, reason);
        LIST_REMOVE(peers, monitor->peers, peer);
        free_and_null(peer->name);
        free_and_null(peer);

        bc_log_debugf("Removed monitor peer '%d' to monitor '%s'", peer_id, monitor->object_path);
        return sd_bus_reply_method_return(m, "");
}


/**********************************
 ********* Monitor events *********
 **********************************/

static sd_bus_message *assemble_unit_new_signal(
                Monitor *monitor, const char *node, const char *unit, const char *reason);
static sd_bus_message *assemble_unit_removed_signal(
                Monitor *monitor, const char *node, const char *unit, const char *reason);
static sd_bus_message *assemble_unit_property_changed_signal(
                Monitor *monitor, const char *node, const char *unit, const char *interface, sd_bus_message *m);
static sd_bus_message *assemble_unit_state_changed_signal(
                Monitor *monitor,
                const char *node,
                const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason);

int monitor_on_unit_property_changed(
                void *userdata, const char *node, const char *unit, const char *interface, sd_bus_message *m) {
        Monitor *monitor = (Monitor *) userdata;
        Controller *controller = monitor->controller;

        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;

        sig = assemble_unit_property_changed_signal(monitor, node, unit, interface, m);
        if (sig != NULL) {
                r = sd_bus_send_to(controller->api_bus, sig, monitor->owner, NULL);
                if (r < 0) {
                        bc_log_errorf("Monitor: %s, failed to send UnitPropertyChanged signal to monitor owner: %s",
                                      monitor->object_path,
                                      strerror(-r));
                }
                sd_bus_message_unrefp(&sig);
                sig = NULL;
        }

        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                sig = assemble_unit_property_changed_signal(monitor, node, unit, interface, m);
                if (sig != NULL) {
                        r = sd_bus_send_to(controller->api_bus, sig, peer->name, NULL);
                        if (r < 0) {
                                bc_log_errorf("Monitor: %s, failed to send UnitPropertyChanged signal to peer '%s': %s",
                                              monitor->object_path,
                                              peer->name,
                                              strerror(-r));
                        }
                        sd_bus_message_unrefp(&sig);
                        sig = NULL;
                }
        }

        return 0;
}

int monitor_on_unit_new(void *userdata, const char *node, const char *unit, const char *reason) {
        Monitor *monitor = (Monitor *) userdata;
        Controller *controller = monitor->controller;

        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;

        sig = assemble_unit_new_signal(monitor, node, unit, reason);
        if (sig != NULL) {
                r = sd_bus_send_to(controller->api_bus, sig, monitor->owner, NULL);
                if (r < 0) {
                        bc_log_errorf("Monitor: %s, failed to send UnitNew signal to monitor owner: %s",
                                      monitor->object_path,
                                      strerror(-r));
                }
                sd_bus_message_unrefp(&sig);
                sig = NULL;
        }

        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                sig = assemble_unit_new_signal(monitor, node, unit, reason);
                if (sig != NULL) {
                        r = sd_bus_send_to(controller->api_bus, sig, peer->name, NULL);
                        if (r < 0) {
                                bc_log_errorf("Monitor: %s, failed to send signal to peer '%s': %s",
                                              monitor->object_path,
                                              peer->name,
                                              strerror(-r));
                        }
                        sd_bus_message_unrefp(&sig);
                        sig = NULL;
                }
        }

        return 0;
}

int monitor_on_unit_state_changed(
                void *userdata,
                const char *node,
                const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason) {
        Monitor *monitor = (Monitor *) userdata;
        Controller *controller = monitor->controller;

        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;

        sig = assemble_unit_state_changed_signal(monitor, node, unit, active_state, substate, reason);
        if (sig != NULL) {
                r = sd_bus_send_to(controller->api_bus, sig, monitor->owner, NULL);
                if (r < 0) {
                        bc_log_errorf("Monitor: %s, failed to send signal to monitor owner: %s",
                                      monitor->object_path,
                                      strerror(-r));
                }
                sd_bus_message_unrefp(&sig);
                sig = NULL;
        }

        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                sig = assemble_unit_state_changed_signal(monitor, node, unit, active_state, substate, reason);
                if (sig != NULL) {
                        r = sd_bus_send_to(controller->api_bus, sig, peer->name, NULL);
                        if (r < 0) {
                                bc_log_errorf("Monitor: %s, failed to send UnitStateChanged signal to peer '%s': %s",
                                              monitor->object_path,
                                              peer->name,
                                              strerror(-r));
                        }
                        sd_bus_message_unrefp(&sig);
                        sig = NULL;
                }
        }

        return 0;
}

int monitor_on_unit_removed(void *userdata, const char *node, const char *unit, const char *reason) {
        Monitor *monitor = (Monitor *) userdata;
        Controller *controller = monitor->controller;

        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;

        sig = assemble_unit_removed_signal(monitor, node, unit, reason);
        if (sig != NULL) {
                r = sd_bus_send_to(controller->api_bus, sig, monitor->owner, NULL);
                if (r < 0) {
                        bc_log_errorf("Monitor: %s, failed to send signal to monitor owner: %s",
                                      monitor->object_path,
                                      strerror(-r));
                }
                sd_bus_message_unrefp(&sig);
                sig = NULL;
        }

        MonitorPeer *peer = NULL;
        MonitorPeer *next_peer = NULL;
        LIST_FOREACH_SAFE(peers, peer, next_peer, monitor->peers) {
                sig = assemble_unit_removed_signal(monitor, node, unit, reason);
                if (sig != NULL) {
                        r = sd_bus_send_to(controller->api_bus, sig, peer->name, NULL);
                        if (r < 0) {
                                bc_log_errorf("Monitor: %s, failed to send UnitRemoved signal to peer '%s': %s",
                                              monitor->object_path,
                                              peer->name,
                                              strerror(-r));
                        }
                        sd_bus_message_unrefp(&sig);
                        sig = NULL;
                }
        }

        return 0;
}


static sd_bus_message *assemble_unit_new_signal(
                Monitor *monitor, const char *node, const char *unit, const char *reason) {
        Controller *controller = monitor->controller;

        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        int r = sd_bus_message_new_signal(
                        controller->api_bus, &sig, monitor->object_path, MONITOR_INTERFACE, "UnitNew");
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to create UnitNew signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_append(sig, "sss", node, unit, reason);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to append data to UnitNew signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        return steal_pointer(&sig);
}

static sd_bus_message *assemble_unit_removed_signal(
                Monitor *monitor, const char *node, const char *unit, const char *reason) {
        Controller *controller = monitor->controller;

        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        int r = sd_bus_message_new_signal(
                        controller->api_bus, &sig, monitor->object_path, MONITOR_INTERFACE, "UnitRemoved");
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to create UnitRemoved signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_append(sig, "sss", node, unit, reason);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to append data to UnitRemoved signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        return steal_pointer(&sig);
}

static sd_bus_message *assemble_unit_property_changed_signal(
                Monitor *monitor, const char *node, const char *unit, const char *interface, sd_bus_message *m) {
        Controller *controller = monitor->controller;

        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        int r = sd_bus_message_new_signal(
                        controller->api_bus,
                        &sig,
                        monitor->object_path,
                        MONITOR_INTERFACE,
                        "UnitPropertiesChanged");
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to create UnitPropertiesChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_append(sig, "sss", node, unit, interface);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to append data to UnitPropertiesChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_skip(m, "ss"); // Skip unit & iface
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to skip unit and interface for UnitPropertiesChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_copy(sig, m, false);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to copy properties for UnitPropertiesChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_rewind(m, false);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to rewind original UnitPropertiesChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        return steal_pointer(&sig);
}

static sd_bus_message *assemble_unit_state_changed_signal(
                Monitor *monitor,
                const char *node,
                const char *unit,
                const char *active_state,
                const char *substate,
                const char *reason) {
        Controller *controller = monitor->controller;

        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        int r = sd_bus_message_new_signal(
                        controller->api_bus, &sig, monitor->object_path, MONITOR_INTERFACE, "UnitStateChanged");
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to create UnitStateChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        r = sd_bus_message_append(sig, "sssss", node, unit, active_state, substate, reason);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to append data to UnitStateChanged signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return NULL;
        }

        return steal_pointer(&sig);
}

static void monitor_emit_peer_removed(void *userdata, MonitorPeer *peer, const char *reason) {
        Monitor *monitor = (Monitor *) userdata;
        Controller *controller = monitor->controller;

        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        int r = sd_bus_message_new_signal(
                        controller->api_bus, &sig, monitor->object_path, MONITOR_INTERFACE, "PeerRemoved");
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to create PeerRemoved signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return;
        }

        r = sd_bus_message_append(sig, "s", reason);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to append data to PeerRemoved signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return;
        }

        r = sd_bus_send_to(controller->api_bus, sig, peer->name, NULL);
        if (r < 0) {
                bc_log_errorf("Monitor: %s, failed to send PeerRemoved signal: %s",
                              monitor->object_path,
                              strerror(-r));
                return;
        }
}
