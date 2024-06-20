/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libbluechi/bus/bus.h"
#include "libbluechi/bus/utils.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/log/log.h"

#include "controller.h"
#include "job.h"
#include "metrics.h"
#include "monitor.h"
#include "node.h"
#include "proxy_monitor.h"
#include <systemd/sd-bus.h>

#define DEBUG_AGENT_MESSAGES 0

static void node_send_agent_subscribe_all(Node *node);
static void node_start_proxy_dependency_all(Node *node);
static int node_run_unit_lifecycle_method(sd_bus_message *m, Node *node, const char *job_type, const char *method);

static int node_method_register(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int node_disconnected(sd_bus_message *message, void *userdata, sd_bus_error *error);
static int node_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_set_unit_properties(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_start_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_stop_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_restart_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_reload_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_passthrough_to_agent(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_method_set_log_level(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error);
static int node_property_get_status(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);
static int node_property_get_peer_ip(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);
static int node_property_get_last_seen(
                sd_bus *bus,
                const char *path,
                const char *interface,
                const char *property,
                sd_bus_message *reply,
                void *userdata,
                sd_bus_error *ret_error);

static const sd_bus_vtable internal_controller_controller_vtable[] = {
        SD_BUS_VTABLE_START(0), SD_BUS_METHOD("Register", "s", "", node_method_register, 0), SD_BUS_VTABLE_END
};

static const sd_bus_vtable node_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", UNIT_INFO_STRUCT_ARRAY_TYPESTRING, node_method_list_units, 0),
        SD_BUS_METHOD("StartUnit", "ss", "o", node_method_start_unit, 0),
        SD_BUS_METHOD("StopUnit", "ss", "o", node_method_stop_unit, 0),
        SD_BUS_METHOD("FreezeUnit", "s", "", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("ThawUnit", "s", "", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("RestartUnit", "ss", "o", node_method_restart_unit, 0),
        SD_BUS_METHOD("ReloadUnit", "ss", "o", node_method_reload_unit, 0),
        SD_BUS_METHOD("GetUnitProperties", "ss", "a{sv}", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("GetUnitProperty", "sss", "v", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("SetUnitProperties", "sba(sv)", "", node_method_set_unit_properties, 0),
        SD_BUS_METHOD("EnableUnitFiles", "asbb", "ba(sss)", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("DisableUnitFiles", "asb", "a(sss)", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("Reload", "", "", node_method_passthrough_to_agent, 0),
        SD_BUS_METHOD("SetLogLevel", "s", "", node_method_set_log_level, 0),
        SD_BUS_PROPERTY("Name", "s", NULL, offsetof(Node, name), SD_BUS_VTABLE_PROPERTY_CONST),
        SD_BUS_PROPERTY("Status", "s", node_property_get_status, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
        SD_BUS_PROPERTY("PeerIp", "s", node_property_get_peer_ip, 0, SD_BUS_VTABLE_PROPERTY_EXPLICIT),
        SD_BUS_PROPERTY("LastSeenTimestamp", "t", node_property_get_last_seen, 0, SD_BUS_VTABLE_PROPERTY_EXPLICIT),
        SD_BUS_VTABLE_END
};

struct ProxyDependency {
        char *unit_name;
        int n_deps;
        LIST_FIELDS(ProxyDependency, deps);
};

static void proxy_dependency_free(struct ProxyDependency *dep) {
        free_and_null(dep->unit_name);
        free_and_null(dep);
}

typedef struct UnitSubscription UnitSubscription;

struct UnitSubscription {
        Subscription *sub;
        LIST_FIELDS(UnitSubscription, subs);
};

typedef struct {
        char *unit;
        LIST_HEAD(UnitSubscription, subs);
        bool loaded;
        UnitActiveState active_state;
        char *substate;
} UnitSubscriptions;

typedef struct {
        char *unit;
} UnitSubscriptionsKey;

static void unit_subscriptions_clear(void *item) {
        UnitSubscriptions *usubs = item;
        free_and_null(usubs->unit);
        free_and_null(usubs->substate);
        assert(LIST_IS_EMPTY(usubs->subs));
}

static uint64_t unit_subscriptions_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const UnitSubscriptions *usubs = item;
        return hashmap_sip(usubs->unit, strlen(usubs->unit), seed0, seed1);
}

static int unit_subscriptions_compare(const void *a, const void *b, UNUSED void *udata) {
        const UnitSubscriptions *usubs_a = a;
        const UnitSubscriptions *usubs_b = b;

        return strcmp(usubs_a->unit, usubs_b->unit);
}


Node *node_new(Controller *controller, const char *name) {
        _cleanup_node_ Node *node = malloc0(sizeof(Node));
        if (node == NULL) {
                return NULL;
        }

        node->ref_count = 1;
        node->controller = controller;
        LIST_INIT(nodes, node);
        LIST_HEAD_INIT(node->outstanding_requests);
        LIST_HEAD_INIT(node->proxy_monitors);
        LIST_HEAD_INIT(node->proxy_dependencies);

        node->unit_subscriptions = hashmap_new(
                        sizeof(UnitSubscriptions),
                        0,
                        0,
                        0,
                        unit_subscriptions_hash,
                        unit_subscriptions_compare,
                        unit_subscriptions_clear,
                        NULL);
        if (node->unit_subscriptions == NULL) {
                return NULL;
        }

        node->last_seen = 0;

        node->name = NULL;
        if (name) {
                node->name = strdup(name);
                if (node->name == NULL) {
                        return NULL;
                }

                int r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, name, &node->object_path);
                if (r < 0) {
                        return NULL;
                }
        }
        node->peer_ip = NULL;

        node->is_shutdown = false;

        return steal_pointer(&node);
}

Node *node_ref(Node *node) {
        node->ref_count++;
        return node;
}

void node_unref(Node *node) {
        node->ref_count--;
        if (node->ref_count != 0) {
                return;
        }

        ProxyMonitor *proxy_monitor = NULL;
        ProxyMonitor *next_proxy_monitor = NULL;
        LIST_FOREACH_SAFE(monitors, proxy_monitor, next_proxy_monitor, node->proxy_monitors) {
                node_remove_proxy_monitor(node, proxy_monitor);
        }


        ProxyDependency *dep = NULL;
        ProxyDependency *next_dep = NULL;
        LIST_FOREACH_SAFE(deps, dep, next_dep, node->proxy_dependencies) {
                proxy_dependency_free(dep);
        }


        node_unset_agent_bus(node);
        sd_bus_slot_unrefp(&node->export_slot);

        hashmap_free(node->unit_subscriptions);

        free_and_null(node->name);
        free_and_null(node->object_path);
        free_and_null(node->peer_ip);
        free(node);
}

void node_shutdown(Node *node) {
        AgentRequest *req = NULL;
        AgentRequest *next_req = NULL;
        node->is_shutdown = true;
        LIST_FOREACH_SAFE(outstanding_requests, req, next_req, node->outstanding_requests) {
                agent_request_cancel(req);
        }
}

bool node_export(Node *node) {
        Controller *controller = node->controller;

        assert(node->name != NULL);

        int r = sd_bus_add_object_vtable(
                        controller->api_bus,
                        &node->export_slot,
                        node->object_path,
                        NODE_INTERFACE,
                        node_vtable,
                        node);
        if (r < 0) {
                bc_log_errorf("Failed to add node vtable: %s", strerror(-r));
                return false;
        }

        return true;
}

static int debug_messages_handler(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        if (node->name) {
                bc_log_infof("Incoming message from node '%s' (fd %d): path: %s, iface: %s, member: %s, signature: '%s'",
                             node->name,
                             sd_bus_get_fd(node->agent_bus),
                             sd_bus_message_get_path(m),
                             sd_bus_message_get_interface(m),
                             sd_bus_message_get_member(m),
                             sd_bus_message_get_signature(m, true));
        } else {
                bc_log_infof("Incoming message from node fd %d: path: %s, iface: %s, member: %s, signature: '%s'",
                             sd_bus_get_fd(node->agent_bus),
                             sd_bus_message_get_path(m),
                             sd_bus_message_get_interface(m),
                             sd_bus_message_get_member(m),
                             sd_bus_message_get_signature(m, true));
        }
        return 0;
}

bool node_has_agent(Node *node) {
        return node->agent_bus != NULL;
}

bool node_is_online(Node *node) {
        return node && node->name && node_has_agent(node);
}


static uint64_t subscription_hashmap_hash(const void *item, UNUSED uint64_t seed0, UNUSED uint64_t seed1) {
        const Subscription * const *subscriptionp = item;
        return (uint64_t) ((uintptr_t) *subscriptionp);
}

static int subscription_hashmap_compare(const void *a, const void *b, UNUSED void *udata) {
        const Subscription * const *subscription_a_p = a;
        const Subscription * const *subscription_b_p = b;
        if ((*subscription_a_p)->monitor == (*subscription_b_p)->monitor) {
                return 0;
        }
        return 1;
}

static struct hashmap *node_compute_unique_monitor_subscriptions(Node *node, const char *unit) {
        struct hashmap *unique_subs = hashmap_new(
                        sizeof(void *), 0, 0, 0, subscription_hashmap_hash, subscription_hashmap_compare, NULL, NULL);
        if (unique_subs == NULL) {
                return NULL;
        }

        const UnitSubscriptionsKey key = { (char *) unit };
        const UnitSubscriptions *usubs = hashmap_get(node->unit_subscriptions, &key);
        if (usubs != NULL) {
                UnitSubscription *usub = NULL;
                UnitSubscription *next_usub = NULL;
                LIST_FOREACH_SAFE(subs, usub, next_usub, usubs->subs) {
                        Subscription *sub = usub->sub;
                        hashmap_set(unique_subs, &sub);
                        if (hashmap_oom(unique_subs)) {
                                bc_log_error("Failed to compute vector of unique monitors, OOM");

                                hashmap_free(unique_subs);
                                unique_subs = NULL;
                                return NULL;
                        }
                }
        }

        /* Only check for wildcards if the unit itself is not one. */
        if (!streq(unit, SYMBOL_WILDCARD)) {
                const UnitSubscriptionsKey wildcard_key = { (char *) SYMBOL_WILDCARD };
                const UnitSubscriptions *usubs_wildcard = hashmap_get(node->unit_subscriptions, &wildcard_key);
                if (usubs_wildcard != NULL) {
                        UnitSubscription *usub = NULL;
                        UnitSubscription *next_usub = NULL;
                        LIST_FOREACH_SAFE(subs, usub, next_usub, usubs_wildcard->subs) {
                                Subscription *sub = usub->sub;
                                hashmap_set(unique_subs, &sub);
                                if (hashmap_oom(unique_subs)) {
                                        bc_log_error("Failed to compute vector of unique monitors, OOM");

                                        hashmap_free(unique_subs);
                                        unique_subs = NULL;
                                        return NULL;
                                }
                        }
                }
        }

        return unique_subs;
}


static int node_match_job_state_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        Controller *controller = node->controller;
        uint32_t bc_job_id = 0;
        const char *state = NULL;

        int r = sd_bus_message_read(m, "us", &bc_job_id, &state);
        if (r < 0) {
                bc_log_errorf("Invalid JobStateChange signal: %s", strerror(-r));
                return 0;
        }

        controller_job_state_changed(controller, bc_job_id, state);
        return 1;
}

static int node_match_unit_properties_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        const char *unit = NULL;
        const char *interface = NULL;

        int r = sd_bus_message_read(m, "ss", &unit, &interface);
        if (r >= 0) {
                r = sd_bus_message_rewind(m, false);
        }
        if (r < 0) {
                bc_log_error("Invalid UnitPropertiesChanged signal");
                return 0;
        }

        struct hashmap *unique_subs = node_compute_unique_monitor_subscriptions(node, unit);
        if (unique_subs != NULL) {
                Subscription **subp = NULL;
                size_t i = 0;
                while (hashmap_iter(unique_subs, &i, (void **) &subp)) {
                        Subscription *sub = *subp;
                        int r = sub->handle_unit_property_changed(sub->monitor, node->name, unit, interface, m);
                        if (r < 0) {
                                bc_log_error("Failed to emit UnitPropertyChanged signal");
                        }
                }
                hashmap_free(unique_subs);
        }

        return 1;
}


static int node_match_unit_new(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        const char *unit = NULL;
        const char *reason = NULL;

        int r = sd_bus_message_read(m, "ss", &unit, &reason);
        if (r < 0) {
                bc_log_errorf("Invalid UnitNew signal: %s", strerror(-r));
                return 0;
        }

        const UnitSubscriptionsKey key = { (char *) unit };
        UnitSubscriptions *usubs = (UnitSubscriptions *) hashmap_get(node->unit_subscriptions, &key);
        if (usubs != NULL) {
                usubs->loaded = true;
                if (is_wildcard(unit)) {
                        usubs->active_state = UNIT_ACTIVE;
                        free(usubs->substate);
                        usubs->substate = strdup("running");
                }
        }

        struct hashmap *unique_subs = node_compute_unique_monitor_subscriptions(node, unit);
        if (unique_subs != NULL) {
                Subscription **subp = NULL;
                size_t i = 0;
                while (hashmap_iter(unique_subs, &i, (void **) &subp)) {
                        Subscription *sub = *subp;
                        int r = sub->handle_unit_new(sub->monitor, node->name, unit, reason);
                        if (r < 0) {
                                bc_log_errorf("Failed to emit UnitNew signal: %s", strerror(-r));
                        }
                }
                hashmap_free(unique_subs);
        }

        return 1;
}

static int node_match_unit_state_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        const char *unit = NULL;
        const char *active_state = NULL;
        const char *substate = NULL;
        const char *reason = NULL;

        int r = sd_bus_message_read(m, "ssss", &unit, &active_state, &substate, &reason);
        if (r < 0) {
                bc_log_errorf("Invalid UnitStateChanged signal: %s", strerror(-r));
                return 0;
        }

        const UnitSubscriptionsKey key = { (char *) unit };
        UnitSubscriptions *usubs = (UnitSubscriptions *) hashmap_get(node->unit_subscriptions, &key);
        if (usubs != NULL) {
                usubs->loaded = true;
                usubs->active_state = active_state_from_string(active_state);
                free(usubs->substate);
                usubs->substate = strdup(substate);
        }

        struct hashmap *unique_subs = node_compute_unique_monitor_subscriptions(node, unit);
        if (unique_subs != NULL) {
                Subscription **subp = NULL;
                size_t i = 0;
                while (hashmap_iter(unique_subs, &i, (void **) &subp)) {
                        Subscription *sub = *subp;
                        int r = sub->handle_unit_state_changed(
                                        sub->monitor, node->name, unit, active_state, substate, reason);
                        if (r < 0) {
                                bc_log_errorf("Failed to emit UnitStateChanged signal: %s", strerror(-r));
                        }
                }
                hashmap_free(unique_subs);
        }

        return 1;
}

static int node_match_unit_removed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                bc_log_errorf("Invalid UnitRemoved signal: %s", strerror(-r));
                return 0;
        }

        const UnitSubscriptionsKey key = { (char *) unit };
        UnitSubscriptions *usubs = (UnitSubscriptions *) hashmap_get(node->unit_subscriptions, &key);
        if (usubs != NULL) {
                usubs->loaded = false;
        }

        struct hashmap *unique_subs = node_compute_unique_monitor_subscriptions(node, unit);
        if (unique_subs != NULL) {
                Subscription **subp = NULL;
                size_t i = 0;
                while (hashmap_iter(unique_subs, &i, (void **) &subp)) {
                        Subscription *sub = *subp;
                        int r = sub->handle_unit_removed(sub->monitor, node->name, unit, "real");
                        if (r < 0) {
                                bc_log_errorf("Failed to emit UnitRemoved signal: %s", strerror(-r));
                        }
                }
                hashmap_free(unique_subs);
        }

        return 1;
}

static int node_match_job_done(UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        Controller *controller = node->controller;
        uint32_t bc_job_id = 0;
        const char *result = NULL;

        int r = sd_bus_message_read(m, "us", &bc_job_id, &result);
        if (r < 0) {
                bc_log_errorf("Invalid JobDone signal: %s", strerror(-r));
                return 0;
        }

        controller_finish_job(controller, bc_job_id, result);
        return 1;
}

static int node_match_heartbeat(UNUSED sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;

        uint64_t now = get_time_micros();
        if (now == 0) {
                bc_log_error("Failed to get current time on heartbeat");
                return 0;
        }

        node->last_seen = now;
        return 1;
}

static ProxyMonitor *node_find_proxy_monitor(Node *node, const char *target_node_name, const char *unit_name) {
        ProxyMonitor *proxy_monitor = NULL;
        LIST_FOREACH(monitors, proxy_monitor, node->proxy_monitors) {
                if (streq(proxy_monitor->target_node->name, target_node_name) &&
                    streq(proxy_monitor->unit_name, unit_name)) {
                        return proxy_monitor;
                }
        }

        return NULL;
}

static int node_on_match_proxy_new(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        Controller *controller = node->controller;

        const char *target_node_name = NULL;
        const char *unit_name = NULL;
        const char *proxy_object_path = NULL;

        int r = sd_bus_message_read(m, "sso", &target_node_name, &unit_name, &proxy_object_path);
        if (r < 0) {
                bc_log_errorf("Invalid arguments in ProxyNew signal: %s", strerror(-r));
                return r;
        }
        bc_log_infof("Node '%s' registered new proxy for unit '%s' on node '%s'",
                     node->name,
                     unit_name,
                     target_node_name);

        _cleanup_proxy_monitor_ ProxyMonitor *monitor = proxy_monitor_new(
                        node, target_node_name, unit_name, proxy_object_path);
        if (monitor == NULL) {
                bc_log_error("Failed to create proxy monitor, OOM");
                return -ENOMEM;
        }

        Node *target_node = controller_find_node(controller, target_node_name);
        if (target_node == NULL) {
                bc_log_error("Proxy requested for non-existing node");
                proxy_monitor_send_error(monitor, "No such node");
                return 0;
        }

        ProxyMonitor *old_monitor = node_find_proxy_monitor(node, target_node_name, unit_name);
        if (old_monitor != NULL) {
                bc_log_warnf("Proxy for '%s' (on '%s') requested, but old proxy already exists, removing it",
                             unit_name,
                             target_node_name);
                node_remove_proxy_monitor(node, old_monitor);
        }

        r = proxy_monitor_set_target_node(monitor, target_node);
        if (r < 0) {
                bc_log_errorf("Failed to add proxy dependency: %s", strerror(-r));
                proxy_monitor_send_error(monitor, "Failed to add proxy dependency");
                return 0;
        }

        /* We now have a valid monitor, add it to the list and enable monitor.
           From this point we should not send errors. */
        controller_add_subscription(controller, monitor->subscription);
        LIST_APPEND(monitors, node->proxy_monitors, proxy_monitor_ref(monitor));

        /* TODO: What about !!node_is_online(target_node) ? Tell now or wait for it to connect? */

        return 0;
}

void node_remove_proxy_monitor(Node *node, ProxyMonitor *monitor) {
        Controller *controller = node->controller;

        proxy_monitor_close(monitor);

        controller_remove_subscription(controller, monitor->subscription);
        LIST_REMOVE(monitors, node->proxy_monitors, monitor);

        proxy_monitor_unref(monitor);
}

static int node_on_match_proxy_removed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;
        const char *target_node_name = NULL;
        const char *unit_name = NULL;

        int r = sd_bus_message_read(m, "ss", &target_node_name, &unit_name);
        if (r < 0) {
                bc_log_errorf("Invalid arguments in ProxyRemoved signal: %s", strerror(-r));
                return r;
        }
        bc_log_infof("Node '%s' unregistered proxy for unit '%s' on node '%s'",
                     node->name,
                     unit_name,
                     target_node_name);

        ProxyMonitor *proxy_monitor = node_find_proxy_monitor(node, target_node_name, unit_name);
        if (proxy_monitor == NULL) {
                bc_log_error("Got ProxyRemoved for unknown monitor");
                return 0;
        }

        node_remove_proxy_monitor(node, proxy_monitor);
        return 0;
}

bool node_set_agent_bus(Node *node, sd_bus *bus) {
        int r = 0;

        if (node->agent_bus != NULL) {
                bc_log_error("Error: Trying to add two agents for a node");
                return false;
        }

        node->agent_bus = sd_bus_ref(bus);
        // If getting peer IP fails, only log and proceed as normal.
        _cleanup_free_ char *peer_ip = NULL;
        uint16_t peer_port = 0;
        r = get_peer_address(node->agent_bus, &peer_ip, &peer_port);
        if (r < 0) {
                bc_log_errorf("Failed to get peer IP: %s", strerror(-r));
        } else {
                node->peer_ip = steal_pointer(&peer_ip);
        }

        if (node->name == NULL) {
                // We only connect to this on the unnamed nodes so register
                // can be called. We can't reconnect it during migration.
                r = sd_bus_add_object_vtable(
                                bus,
                                &node->internal_controller_slot,
                                INTERNAL_CONTROLLER_OBJECT_PATH,
                                INTERNAL_CONTROLLER_INTERFACE,
                                internal_controller_controller_vtable,
                                node);
                if (r < 0) {
                        node_unset_agent_bus(node);
                        bc_log_errorf("Failed to add peer bus vtable: %s", strerror(-r));
                        return false;
                }
        } else {
                // Only listen to signals on named nodes
                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "JobDone",
                                node_match_job_done,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add JobDone peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "JobStateChanged",
                                node_match_job_state_changed,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add JobStateChanged peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "UnitPropertiesChanged",
                                node_match_unit_properties_changed,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add UnitPropertiesChanged peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "UnitNew",
                                node_match_unit_new,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add UnitNew peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "UnitStateChanged",
                                node_match_unit_state_changed,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add UnitStateChanged peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "UnitRemoved",
                                node_match_unit_removed,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add UnitRemoved peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "ProxyNew",
                                node_on_match_proxy_new,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add ProxyNew peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "ProxyRemoved",
                                node_on_match_proxy_removed,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add ProxyNew peer bus match: %s", strerror(-r));
                        return false;
                }

                r = sd_bus_emit_properties_changed(
                                node->controller->api_bus, node->object_path, NODE_INTERFACE, "Status", NULL);
                if (r < 0) {
                        bc_log_errorf("Failed to emit status property changed: %s", strerror(-r));
                }

                r = sd_bus_match_signal(
                                bus,
                                NULL,
                                NULL,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                AGENT_HEARTBEAT_SIGNAL_NAME,
                                node_match_heartbeat,
                                node);
                if (r < 0) {
                        bc_log_errorf("Failed to add heartbeat signal match: %s", strerror(-r));
                        return false;
                }
        }

        r = sd_bus_match_signal_async(
                        bus,
                        &node->disconnect_slot,
                        "org.freedesktop.DBus.Local",
                        "/org/freedesktop/DBus/Local",
                        "org.freedesktop.DBus.Local",
                        "Disconnected",
                        node_disconnected,
                        NULL,
                        node);
        if (r < 0) {
                node_unset_agent_bus(node);
                bc_log_errorf("Failed to request match for Disconnected message: %s", strerror(-r));
                return false;
        }

        if (DEBUG_AGENT_MESSAGES) {
                sd_bus_add_filter(bus, NULL, debug_messages_handler, node);
        }


        /* Register any active subscriptions with new agent */
        node_send_agent_subscribe_all(node);

        /* Register any active dependencies with new agent */
        node_start_proxy_dependency_all(node);

        return true;
}

void node_unset_agent_bus(Node *node) {
        bool was_online = node->name && node_has_agent(node);

        sd_bus_slot_unrefp(&node->disconnect_slot);
        node->disconnect_slot = NULL;

        sd_bus_slot_unrefp(&node->internal_controller_slot);
        node->internal_controller_slot = NULL;

        sd_bus_slot_unrefp(&node->metrics_matching_slot);
        node->metrics_matching_slot = NULL;

        sd_bus_unrefp(&node->agent_bus);
        node->agent_bus = NULL;

        free_and_null(node->peer_ip);

        if (was_online) {
                int r = sd_bus_emit_properties_changed(
                                node->controller->api_bus, node->object_path, NODE_INTERFACE, "Status", NULL);
                if (r < 0) {
                        bc_log_errorf("Failed to emit status property changed: %s", strerror(-r));
                }
        }
}

/* org.eclipse.bluechi.internal.Controller.Register(in s name)) */
static int node_method_register(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        Controller *controller = node->controller;
        char *name = NULL;
        _cleanup_free_ char *description = NULL;

        /* Once we're not anonymous, don't allow register calls */
        if (node->name != NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_ADDRESS_IN_USE, "Can't register twice");
        }

        /* Read the parameters */
        int r = sd_bus_message_read(m, "s", &name);
        if (r < 0) {
                bc_log_errorf("Failed to parse parameters: %s", strerror(-r));
                return r;
        }

        Node *named_node = controller_find_node(controller, name);
        if (named_node == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_SERVICE_UNKNOWN, "Unexpected node name");
        }

        if (node_has_agent(named_node)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_ADDRESS_IN_USE, "The node is already connected");
        }

        named_node->last_seen = get_time_micros();

        r = asprintf(&description, "node-%s", name);
        if (r >= 0) {
                (void) sd_bus_set_description(node->agent_bus, description);
        }

        /* Migrate the agent connection to the named node */
        _cleanup_sd_bus_ sd_bus *agent_bus = sd_bus_ref(node->agent_bus);
        if (!node_set_agent_bus(named_node, agent_bus)) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Internal error: Couldn't set agent bus");
        }

        if (controller->metrics_enabled) {
                node_enable_metrics(named_node);
        }

        node_unset_agent_bus(node);

        /* update number of online nodes and check the new system state */
        controller_check_system_status(controller, controller->number_of_nodes_online++);

        bc_log_infof("Registered managed node from fd %d as '%s'", sd_bus_get_fd(agent_bus), name);

        return sd_bus_reply_method_return(m, "");
}

static int node_disconnected(UNUSED sd_bus_message *message, void *userdata, UNUSED sd_bus_error *error) {
        Node *node = userdata;

        node_disconnect(node);

        return 0;
}

void node_disconnect(Node *node) {
        Controller *controller = node->controller;
        void *item = NULL;
        size_t i = 0;

        /* Send virtual unit remove and state change for any reported loaded units */
        while (hashmap_iter(node->unit_subscriptions, &i, &item)) {
                UnitSubscriptions *usubs = item;
                bool send_state_change = false;

                if (!usubs->loaded) {
                        continue;
                }

                if (usubs->active_state >= 0 && usubs->active_state != UNIT_INACTIVE) {
                        /* We previously reported an not-inactive valid state, send a virtual inactive state */
                        usubs->active_state = UNIT_INACTIVE;
                        free(usubs->substate);
                        usubs->substate = strdup("agent-offline");
                        send_state_change = true;
                }

                usubs->loaded = false;

                int r = 0;
                if (send_state_change) {
                        struct hashmap *unique_subs = node_compute_unique_monitor_subscriptions(
                                        node, usubs->unit);
                        if (unique_subs != NULL) {

                                Subscription **subp = NULL;
                                size_t s = 0;
                                while (hashmap_iter(unique_subs, &s, (void **) &subp)) {
                                        Subscription *sub = *subp;
                                        r = sub->handle_unit_state_changed(
                                                        sub->monitor,
                                                        node->name,
                                                        usubs->unit,
                                                        active_state_to_string(usubs->active_state),
                                                        usubs->substate,
                                                        "virtual");
                                        if (r < 0) {
                                                bc_log_error("Failed to emit UnitStateChanged signal");
                                        }
                                }
                                hashmap_free(unique_subs);
                        }
                }


                struct hashmap *unique_subs = node_compute_unique_monitor_subscriptions(node, usubs->unit);
                if (unique_subs != NULL) {

                        Subscription **subp = NULL;
                        size_t s = 0;
                        while (hashmap_iter(unique_subs, &s, (void **) &subp)) {
                                Subscription *sub = *subp;
                                r = sub->handle_unit_removed(sub->monitor, node->name, usubs->unit, "virtual");
                                if (r < 0) {
                                        bc_log_error("Failed to emit UnitRemoved signal");
                                }
                        }
                        hashmap_free(unique_subs);
                }
        }

        ProxyMonitor *proxy_monitor = NULL;
        ProxyMonitor *next_proxy_monitor = NULL;
        LIST_FOREACH_SAFE(monitors, proxy_monitor, next_proxy_monitor, node->proxy_monitors) {
                node_remove_proxy_monitor(node, proxy_monitor);
        }

        /* Remove anonymous nodes when they disconnect */
        if (node->name == NULL) {
                bc_log_info("Anonymous node disconnected");
                controller_remove_node(controller, node);
        } else {
                bc_log_infof("Node '%s' disconnected", node->name);
                /* Remove all jobs associated with the registered node that got disconnected. */
                if (!LIST_IS_EMPTY(controller->jobs)) {
                        Job *job = NULL;
                        Job *next_job = NULL;
                        LIST_FOREACH_SAFE(jobs, job, next_job, controller->jobs) {
                                if (strcmp(job->node->name, node->name) == 0) {
                                        bc_log_debugf("Removing job %d from node %s", job->id, job->node->name);
                                        LIST_REMOVE(jobs, controller->jobs, job);
                                        job_unref(job);
                                }
                        }
                }
                node_unset_agent_bus(node);

                /* update number of online nodes and check the new system state */
                controller_check_system_status(controller, controller->number_of_nodes_online--);
        }
}

const char *node_get_status(Node *node) {
        if (node_has_agent(node)) {
                return "online";
        }
        return "offline";
}

static int node_property_get_status(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        return sd_bus_message_append(reply, "s", node_get_status(node));
}

static int node_property_get_peer_ip(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        return sd_bus_message_append(reply, "s", node->peer_ip);
}

static int node_property_get_last_seen(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        return sd_bus_message_append(reply, "t", node->last_seen);
}


AgentRequest *agent_request_ref(AgentRequest *req) {
        req->ref_count++;
        return req;
}

void agent_request_unref(AgentRequest *req) {
        req->ref_count--;
        if (req->ref_count != 0) {
                return;
        }

        if (req->userdata && req->free_userdata) {
                req->free_userdata(req->userdata);
        }
        sd_bus_slot_unrefp(&req->slot);
        sd_bus_message_unrefp(&req->message);

        Node *node = req->node;
        LIST_REMOVE(outstanding_requests, node->outstanding_requests, req);
        node_unref(req->node);
        free(req);
}

int node_create_request(
                AgentRequest **ret,
                Node *node,
                const char *method,
                agent_request_response_t cb,
                void *userdata,
                free_func_t free_userdata) {
        AgentRequest *req = malloc0(sizeof(AgentRequest));
        if (req == NULL) {
                return -ENOMEM;
        }

        int r = sd_bus_message_new_method_call(
                        node->agent_bus,
                        &req->message,
                        BC_AGENT_DBUS_NAME,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        method);
        if (r < 0) {
                free(req);
                req = NULL;
                return r;
        }

        req->ref_count = 1;
        req->node = node_ref(node);
        LIST_INIT(outstanding_requests, req);
        req->cb = cb;
        req->userdata = userdata;
        req->free_userdata = free_userdata;
        req->is_cancelled = false;
        LIST_APPEND(outstanding_requests, node->outstanding_requests, req);

        *ret = req;
        return 0;
}

static int agent_request_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_agent_request_ AgentRequest *req = userdata;
        if (req->is_cancelled) {
                bc_log_debugf("Response received to a cancelled request for node %s. Dropping message.",
                              req->node->name);
                return 0;
        }

        return req->cb(req, m, ret_error);
}

int agent_request_cancel(AgentRequest *r) {
        _cleanup_agent_request_ AgentRequest *req = r;
        req->is_cancelled = true;
        _cleanup_sd_bus_message_ sd_bus_message *m = NULL;
        sd_bus_message_new_method_errorf(req->message, &m, SD_BUS_ERROR_FAILED, "Request cancelled");

        return req->cb(req, m, NULL);
}

int agent_request_start(AgentRequest *req) {
        Node *node = req->node;

        int r = sd_bus_call_async(
                        node->agent_bus,
                        &req->slot,
                        req->message,
                        agent_request_callback,
                        req,
                        BC_DEFAULT_DBUS_TIMEOUT);
        if (r < 0) {
                return r;
        }

        agent_request_ref(req); /* Keep alive while operation is outstanding */
        return 1;
}

AgentRequest *node_request_list_units(
                Node *node, agent_request_response_t cb, void *userdata, free_func_t free_userdata) {
        if (!node_has_agent(node)) {
                return NULL;
        }

        _cleanup_agent_request_ AgentRequest *req = NULL;
        node_create_request(&req, node, "ListUnits", cb, userdata, free_userdata);
        if (req == NULL) {
                return NULL;
        }

        if (agent_request_start(req) < 0) {
                return NULL;
        }

        return steal_pointer(&req);
}

/*************************************************************************
 ********** org.eclipse.bluechi.Node.ListUnits **************************
 ************************************************************************/

static int method_list_units_callback(AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        sd_bus_message *request_message = req->userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(request_message, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message for ListUnits request: %s",
                                strerror(-r));
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to copy the bus message for ListUnits request: %s",
                                strerror(-r));
        }

        return sd_bus_message_send(reply);
}

static int node_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;

        if (node->is_shutdown) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Request not allowed: node is in shutdown state");
        }

        _cleanup_agent_request_ AgentRequest *agent_req = node_request_list_units(
                        node,
                        method_list_units_callback,
                        sd_bus_message_ref(m),
                        (free_func_t) sd_bus_message_unref);
        if (agent_req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "List units not found");
        }

        return 1;
}

/*************************************************************************
 ********** org.eclipse.bluechi.Node.SetUnitProperty ******************
 ************************************************************************/

static int node_method_set_unit_properties_callback(
                AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        sd_bus_message *request_message = req->userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(request_message, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message: %s",
                                strerror(-r));
        }

        return sd_bus_message_send(reply);
}

static int node_method_set_unit_properties(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;
        const char *unit = NULL;
        int runtime = 0;

        if (node->is_shutdown) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Request not allowed: node is in shutdown state");
        }

        int r = sd_bus_message_read(m, "sb", &unit, &runtime);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for unit or runtime: %s",
                                strerror(-r));
        }

        _cleanup_agent_request_ AgentRequest *req = NULL;
        r = node_create_request(
                        &req,
                        node,
                        "SetUnitProperties",
                        node_method_set_unit_properties_callback,
                        sd_bus_message_ref(m),
                        (free_func_t) sd_bus_message_unref);
        if (req == NULL) {
                sd_bus_message_unref(m);

                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to create an agent request: %s", strerror(-r));
        }

        r = sd_bus_message_append(req->message, "sb", unit, runtime);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to append unit and runtime to the message: %s",
                                strerror(-r));
        }

        r = sd_bus_message_copy(req->message, m, false);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to copy a message: %s", strerror(-r));
        }

        r = agent_request_start(req);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to call the method to start the node: %s",
                                strerror(-r));
        }

        return 1;
}

static int node_method_passthrough_to_agent_callback(
                AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        sd_bus_message *request_message = req->userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                return sd_bus_reply_method_error(request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(request_message, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to create a reply message: %s",
                                strerror(-r));
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                request_message,
                                SD_BUS_ERROR_FAILED,
                                "Failed to copy a reply message: %s",
                                strerror(-r));
        }

        return sd_bus_message_send(reply);
}

static int node_method_passthrough_to_agent(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Node *node = userdata;

        if (node->is_shutdown) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Request not allowed: node is in shutdown state");
        }

        _cleanup_agent_request_ AgentRequest *req = NULL;
        int r = node_create_request(
                        &req,
                        node,
                        sd_bus_message_get_member(m),
                        node_method_passthrough_to_agent_callback,
                        sd_bus_message_ref(m),
                        (free_func_t) sd_bus_message_unref);
        if (req == NULL) {
                sd_bus_message_unref(m);

                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to create an agent request: %s", strerror(-r));
        }

        r = sd_bus_message_copy(req->message, m, true);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to copy a reply message: %s", strerror(-r));
        }

        r = agent_request_start(req);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to call the method to start the node: %s",
                                strerror(-r));
        }

        return 1;
}

/* Keep track of data related to setting up a job. For example calling
   the initial agent request before we know the job is actually going to
   happen. */
typedef struct {
        int ref_count;
        sd_bus_message *request_message;
        Job *job;
} JobSetup;

static JobSetup *job_setup_ref(JobSetup *setup) {
        setup->ref_count++;
        return setup;
}

static void job_setup_unref(JobSetup *setup) {
        setup->ref_count--;
        if (setup->ref_count != 0) {
                return;
        }

        job_unrefp(&setup->job);
        sd_bus_message_unrefp(&setup->request_message);
        free(setup);
}

DEFINE_CLEANUP_FUNC(JobSetup, job_setup_unref)
#define _cleanup_job_setup_ _cleanup_(job_setup_unrefp)

static JobSetup *job_setup_new(sd_bus_message *request_message, Node *node, const char *unit, const char *type) {
        _cleanup_job_setup_ JobSetup *setup = malloc0(sizeof(JobSetup));
        if (setup == NULL) {
                return NULL;
        }

        setup->ref_count = 1;
        setup->request_message = sd_bus_message_ref(request_message);
        setup->job = job_new(node, unit, type);
        if (setup->job == NULL) {
                NULL;
        }

        return steal_pointer(&setup);
}

static int unit_lifecycle_method_callback(AgentRequest *req, sd_bus_message *m, UNUSED sd_bus_error *ret_error) {
        Node *node = req->node;
        Controller *controller = node->controller;
        JobSetup *setup = req->userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(setup->request_message, sd_bus_message_get_error(m));
        }

        if (!controller_add_job(controller, setup->job)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Failed to add a job");
        }

        return sd_bus_reply_method_return(setup->request_message, "o", setup->job->object_path);
}

static int node_run_unit_lifecycle_method(
                sd_bus_message *m, Node *node, const char *job_type, const char *method) {
        const char *unit = NULL;
        const char *mode = NULL;
        uint64_t start_time = get_time_micros();

        if (node->is_shutdown) {
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Request not allowed: node is in shutdown state");
        }

        int r = sd_bus_message_read(m, "ss", &unit, &mode);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for unit or mode: %s",
                                strerror(-r));
        }

        _cleanup_job_setup_ JobSetup *setup = job_setup_new(m, node, unit, job_type);
        if (setup == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NO_MEMORY, "Out of memory");
        }

        if (node->controller->metrics_enabled) {
                setup->job->job_start_micros = start_time;
        }

        _cleanup_agent_request_ AgentRequest *req = NULL;
        r = node_create_request(
                        &req,
                        node,
                        method,
                        unit_lifecycle_method_callback,
                        job_setup_ref(setup),
                        (free_func_t) job_setup_unref);
        if (req == NULL) {
                job_setup_unref(setup);

                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to create an agent request: %s", strerror(-r));
        }

        r = sd_bus_message_append(req->message, "ssu", unit, mode, setup->job->id);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to append unit, mode, and job ID to the message: %s",
                                strerror(-r));
        }

        r = agent_request_start(req);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to call the method to start the node: %s",
                                strerror(-r));
        }

        return 1;
}


/*************************************************************************
 ********** org.eclipse.bluechi.Node.StartUnit **************************
 ************************************************************************/

static int node_method_start_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "start", "StartUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.Node.StopUnit ***************************
 ************************************************************************/


static int node_method_stop_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "stop", "StopUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.Node.RestartUnit ************************
 ************************************************************************/

static int node_method_restart_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "restart", "RestartUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.Node.ReloadUnit **************************
 ************************************************************************/

static int node_method_reload_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return node_run_unit_lifecycle_method(m, (Node *) userdata, "reload", "ReloadUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.Node.SetLogLevel *******************
 ************************************************************************/

static int node_method_set_log_level(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *level = NULL;
        Node *node = (Node *) userdata;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *sub_m = NULL;

        int r = sd_bus_message_read(m, "s", &level);
        if (r < 0) {
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_INVALID_ARGS,
                                "Invalid argument for the log-level: %s",
                                strerror(-r));
        }
        LogLevel loglevel = string_to_log_level(level);
        if (loglevel == LOG_LEVEL_INVALID) {
                r = sd_bus_reply_method_return(m, "");
                if (r < 0) {
                        return sd_bus_reply_method_errorf(
                                        m,
                                        SD_BUS_ERROR_INVALID_ARGS,
                                        "Invalid argument for the log level invalid");
                }
        }
        r = sd_bus_call_method(
                        node->agent_bus,
                        BC_AGENT_DBUS_NAME,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "SetLogLevel",
                        &error,
                        &sub_m,
                        "s",
                        level);
        if (r < 0) {
                bc_log_errorf("Failed to set log level call: %s", error.message);
                sd_bus_error_free(&error);
                return sd_bus_reply_method_errorf(
                                m,
                                SD_BUS_ERROR_FAILED,
                                "Failed to call a method to set the log level: %s",
                                strerror(-r));
        }
        return sd_bus_reply_method_return(m, "");
}

static int send_agent_simple_message(Node *node, const char *method, const char *arg) {
        _cleanup_sd_bus_message_ sd_bus_message *m = NULL;
        int r = sd_bus_message_new_method_call(
                        node->agent_bus,
                        &m,
                        BC_AGENT_DBUS_NAME,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        method);
        if (r < 0) {
                return r;
        }

        if (arg != NULL) {
                r = sd_bus_message_append(m, "s", arg);
                if (r < 0) {
                        return r;
                }
        }

        return sd_bus_send(node->agent_bus, m, NULL);
}

static void node_send_agent_subscribe(Node *node, const char *unit) {
        if (!node_has_agent(node)) {
                return;
        }

        int r = send_agent_simple_message(node, "Subscribe", unit);
        if (r < 0) {
                bc_log_error("Failed to subscribe w/ agent");
        }
}


static void node_send_agent_unsubscribe(Node *node, const char *unit) {
        if (!node_has_agent(node)) {
                return;
        }

        int r = send_agent_simple_message(node, "Unsubscribe", unit);
        if (r < 0) {
                bc_log_error("Failed to unsubscribe w/ agent");
        }
}

/* Resubscribe to all subscriptions */
static void node_send_agent_subscribe_all(Node *node) {
        void *item = NULL;
        size_t i = 0;

        while (hashmap_iter(node->unit_subscriptions, &i, &item)) {
                UnitSubscriptions *usubs = item;
                node_send_agent_subscribe(node, usubs->unit);
        }
}

void node_subscribe(Node *node, Subscription *sub) {
        SubscribedUnit *sub_unit = NULL;
        SubscribedUnit *next_sub_unit = NULL;
        LIST_FOREACH_SAFE(units, sub_unit, next_sub_unit, sub->subscribed_units) {
                const UnitSubscriptionsKey key = { sub_unit->name };
                UnitSubscriptions *usubs = NULL;

                _cleanup_free_ UnitSubscription *usub = malloc0(sizeof(UnitSubscription));
                if (usub == NULL) {
                        bc_log_error("Failed to subscribe to unit, OOM");
                        return;
                }
                usub->sub = sub;

                usubs = (UnitSubscriptions *) hashmap_get(node->unit_subscriptions, &key);
                if (usubs == NULL) {
                        UnitSubscriptions v = { NULL, NULL, false, _UNIT_ACTIVE_STATE_INVALID, NULL };
                        v.unit = strdup(key.unit);
                        if (v.unit == NULL) {
                                bc_log_error("Failed to subscribe to unit, OOM");
                                return;
                        }

                        usubs = (UnitSubscriptions *) hashmap_set(node->unit_subscriptions, &v);
                        if (usubs == NULL && hashmap_oom(node->unit_subscriptions)) {
                                free(v.unit);
                                bc_log_error("Failed to subscribe to unit, OOM");
                                return;
                        }

                        /* First sub to this unit, pass to agent */
                        node_send_agent_subscribe(node, sub_unit->name);

                        usubs = (UnitSubscriptions *) hashmap_get(node->unit_subscriptions, &key);
                }

                LIST_APPEND(subs, usubs->subs, steal_pointer(&usub));

                /* We know this is loaded, so we won't get notified from
                   the agent, instead send a virtual event here. */
                if (usubs->loaded) {
                        int r = sub->handle_unit_new(sub->monitor, node->name, sub_unit->name, "virtual");
                        if (r < 0) {
                                bc_log_error("Failed to emit UnitNew signal");
                        }

                        if (usubs->active_state >= 0) {
                                r = sub->handle_unit_state_changed(
                                                sub->monitor,
                                                node->name,
                                                sub_unit->name,
                                                active_state_to_string(usubs->active_state),
                                                usubs->substate ? usubs->substate : "invalid",
                                                "virtual");
                                if (r < 0) {
                                        bc_log_error("Failed to emit UnitNew signal");
                                }
                        }
                }
        }
}

void node_unsubscribe(Node *node, Subscription *sub) {
        SubscribedUnit *sub_unit = NULL;
        SubscribedUnit *next_sub_unit = NULL;
        LIST_FOREACH_SAFE(units, sub_unit, next_sub_unit, sub->subscribed_units) {
                UnitSubscriptionsKey key = { sub_unit->name };
                UnitSubscriptions *usubs = NULL;
                UnitSubscription *usub = NULL;
                UnitSubscription *found = NULL;
                UnitSubscriptions *deleted = NULL;

                /* NOTE: If there are errors during subscribe we may still
                   call unsubscribe, so this must silently handle the
                   case of too many unsubscribes. */

                usubs = (UnitSubscriptions *) hashmap_get(node->unit_subscriptions, &key);
                if (usubs == NULL) {
                        continue;
                }

                LIST_FOREACH(subs, usub, usubs->subs) {
                        if (usub->sub == sub) {
                                found = usub;
                                break;
                        }
                }

                if (found == NULL) {
                        continue;
                }

                LIST_REMOVE(subs, usubs->subs, found);
                free_and_null(found);

                if (LIST_IS_EMPTY(usubs->subs)) {
                        /* Last subscription for this unit, tell agent */
                        node_send_agent_unsubscribe(node, sub_unit->name);
                        deleted = (UnitSubscriptions *) hashmap_delete(node->unit_subscriptions, &key);
                        if (deleted) {
                                unit_subscriptions_clear(deleted);
                        }
                }
        }
}

static void node_start_proxy_dependency(Node *node, ProxyDependency *dep) {
        if (!node_has_agent(node)) {
                return;
        }

        bc_log_infof("Starting dependency %s on node %s", dep->unit_name, node->name);

        int r = send_agent_simple_message(node, "StartDep", dep->unit_name);
        if (r < 0) {
                bc_log_error("Failed to send StartDep to agent");
        }
}

static void node_start_proxy_dependency_all(Node *node) {
        ProxyDependency *dep = NULL;
        LIST_FOREACH(deps, dep, node->proxy_dependencies) {
                node_start_proxy_dependency(node, dep);
        }
}

static void node_stop_proxy_dependency(Node *node, ProxyDependency *dep) {
        if (!node_has_agent(node)) {
                return;
        }

        bc_log_infof("Stopping dependency %s on node %s", dep->unit_name, node->name);

        int r = send_agent_simple_message(node, "StopDep", dep->unit_name);
        if (r < 0) {
                bc_log_error("Failed to send StopDep to agent");
        }
}

static struct ProxyDependency *node_find_proxy_dependency(Node *node, const char *unit_name) {
        ProxyDependency *dep = NULL;
        LIST_FOREACH(deps, dep, node->proxy_dependencies) {
                if (streq(dep->unit_name, unit_name)) {
                        return dep;
                }
        }

        return NULL;
}

int node_add_proxy_dependency(Node *node, const char *unit_name) {
        ProxyDependency *dep = NULL;

        dep = node_find_proxy_dependency(node, unit_name);
        if (dep) {
                dep->n_deps++;
                /* Always start, if the dep service was stopped by
                   the target service stopping */
                node_start_proxy_dependency(node, dep);
                return 0;
        }

        _cleanup_free_ char *unit_name_copy = strdup(unit_name);
        if (unit_name_copy == NULL) {
                return -ENOMEM;
        }

        dep = malloc0(sizeof(ProxyDependency));
        if (dep == NULL) {
                return -ENOMEM;
        }

        dep->unit_name = steal_pointer(&unit_name_copy);
        dep->n_deps = 1;
        LIST_APPEND(deps, node->proxy_dependencies, dep);

        node_start_proxy_dependency(node, dep);

        return 0;
}

int node_remove_proxy_dependency(Node *node, const char *unit_name) {
        ProxyDependency *dep = NULL;
        dep = node_find_proxy_dependency(node, unit_name);
        if (!dep) {
                return -ENOENT;
        }

        dep->n_deps--;

        if (dep->n_deps == 0) {
                /* Only stop on the last dep */
                node_stop_proxy_dependency(node, dep);

                LIST_REMOVE(deps, node->proxy_dependencies, dep);
                proxy_dependency_free(dep);
        }

        return 0;
}

int node_method_get_unit_uint64_property_sync(Node *node, char *unit, char *property, uint64_t *value) {
        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        r = sd_bus_call_method(
                        node->agent_bus,
                        BC_AGENT_DBUS_NAME,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "GetUnitProperty",
                        &error,
                        &message,
                        "sss",
                        unit,
                        "org.freedesktop.systemd1.Unit",
                        property);
        if (r < 0) {
                bc_log_errorf("Failed to issue GetUnitProperty call: %s", error.message);
                sd_bus_error_free(&error);
                return r;
        }

        r = sd_bus_message_enter_container(message, SD_BUS_TYPE_VARIANT, "t");
        if (r < 0) {
                bc_log_errorf("Failed to parse response message: %s", strerror(-r));
                return r;
        }

        r = sd_bus_message_read_basic(message, SD_BUS_TYPE_UINT64, value);
        if (r < 0) {
                bc_log_errorf("Failed to parse response message: %s", strerror(-r));
                return r;
        }

        r = sd_bus_message_exit_container(message);
        if (r < 0) {
                bc_log_errorf("Failed to parse response message: %s", strerror(-r));
                return r;
        }

        return 0;
}

void node_enable_metrics(Node *node) {
        if (!node_has_agent(node)) {
                return;
        }

        int r = send_agent_simple_message(node, "EnableMetrics", NULL);
        if (r < 0) {
                bc_log_error("Failed to enable metrics on agent");
        }

        if (!metrics_node_signal_matching_register(node)) {
                bc_log_error("Failed to enable metrics on agent");
        }
}

void node_disable_metrics(Node *node) {
        if (!node_has_agent(node)) {
                return;
        }

        int r = send_agent_simple_message(node, "DisableMetrics", NULL);
        if (r < 0) {
                bc_log_error("Failed to disable metrics on agent");
        }

        sd_bus_slot_unrefp(&node->metrics_matching_slot);
        node->metrics_matching_slot = NULL;
}
