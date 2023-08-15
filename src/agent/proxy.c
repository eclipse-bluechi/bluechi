/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "libbluechi/log/log.h"

#include "agent.h"
#include "proxy.h"


static int proxy_service_method_error(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int proxy_service_method_target_state_changed(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int proxy_service_method_target_new(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);
static int proxy_service_method_target_removed(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

static const sd_bus_vtable proxy_service_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("Error", "s", "", proxy_service_method_error, 0),
        SD_BUS_METHOD("TargetNew", "s", "", proxy_service_method_target_new, 0),
        SD_BUS_METHOD("TargetStateChanged", "sss", "", proxy_service_method_target_state_changed, 0),
        SD_BUS_METHOD("TargetRemoved", "s", "", proxy_service_method_target_removed, 0),
        SD_BUS_VTABLE_END
};

/* This is called by the manager when the service that is the target
 * of a proxy changes state.
 */

static void proxy_service_initial_state_reached(ProxyService *proxy, bool success) {
        Agent *agent = proxy->agent;
        int r = 0;

        assert(proxy->request_message != NULL);

        if (success) {
                bc_log_infof("Replying to %s successfully", proxy->local_service_name);
                proxy->sent_successful_ready = true;
                r = sd_bus_reply_method_return(proxy->request_message, "");
        } else {
                bc_log_infof("Replying to %s with failure", proxy->local_service_name);
                r = sd_bus_reply_method_errorf(
                                proxy->request_message,
                                BC_BUS_ERROR_ACTIVATION_FAILED,
                                "Proxy service failed to start");
        }

        if (r < 0) {
                bc_log_errorf("Failed to send reply to proxy request: %s", strerror(-r));
        }

        sd_bus_message_unrefp(&proxy->request_message);
        proxy->request_message = NULL;

        if (!proxy->sent_successful_ready) {
                agent_remove_proxy(agent, proxy, true);
        }
}

static void proxy_service_target_stopped(ProxyService *proxy) {
        Agent *agent = proxy->agent;

        bc_log_infof("Proxy for %s stopping due to target stopped", proxy->local_service_name);

        assert(proxy->request_message == NULL);
        agent_remove_proxy(agent, proxy, true);
}

static int proxy_service_method_target_state_changed(
                sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_service_ ProxyService *proxy = proxy_service_ref((ProxyService *) userdata);
        Agent *agent = proxy->agent;

        if (agent == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }


        const char *active_state_str = NULL;
        const char *substate = NULL;
        const char *reason = NULL;
        int r = sd_bus_message_read(m, "sss", &active_state_str, &substate, &reason);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        UnitActiveState active_state = active_state_from_string(active_state_str);

        bc_log_debugf("Proxy service '%s' got TargetStateChanged from manager: %s %s %s",
                      proxy->local_service_name,
                      active_state_str,
                      substate,
                      reason);

        if (proxy->request_message) {
                /* We're waiting for the initial start-or-fail to report back
                 *
                 * There are two main things that can happen here.
                 *
                 * Either the state eventually changes to any kind of
                 * active state. Then we consider the target service
                 * to have started, and we report that, and switch to waiting
                 * for it to stop. It is possible that we get a virtual
                 * active state change because the service was already running,
                 * but this is fine here.
                 *
                 * Alternatively, we get a message that says the state
                 * switched to failed, or inactive. This means the service
                 * stopped, which we report as a failure to start the target.
                 * However, here we have to be careful about virtual inactive
                 * events, as we may get a virtual inactive for the current
                 * state before then getting a real transition to the new state.
                 *
                 * In addition, it is possible that we never ever change state, because
                 * the service fails to parse, for instance. If so, we hit the (real) removed
                 * event before seeing any activation. See below for that.
                 */

                if (active_state == UNIT_ACTIVE) {
                        proxy_service_initial_state_reached(proxy, true);
                } else if ((active_state == UNIT_FAILED || active_state == UNIT_INACTIVE) &&
                           streq(reason, "real")) {
                        proxy_service_initial_state_reached(proxy, false);
                }
        } else if (proxy->sent_successful_ready) {
                /* We reached the initial state and are now monitoring for the target to exit */
                if ((active_state == UNIT_FAILED || active_state == UNIT_INACTIVE) && streq(reason, "real")) {
                        proxy_service_target_stopped(proxy);
                }
        }

        return sd_bus_reply_method_return(m, "");
}

static int proxy_service_method_target_new(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_service_ ProxyService *proxy = proxy_service_ref((ProxyService *) userdata);
        Agent *agent = proxy->agent;
        if (agent == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        const char *reason = NULL;
        int r = sd_bus_message_read(m, "s", &reason);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        bc_log_debugf("Proxy service '%s' got TargetNew from manager: %s", proxy->local_service_name, reason);

        return sd_bus_reply_method_return(m, "");
}

static int proxy_service_method_target_removed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_service_ ProxyService *proxy = proxy_service_ref((ProxyService *) userdata);
        Agent *agent = proxy->agent;
        if (agent == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        const char *reason = NULL;
        int r = sd_bus_message_read(m, "s", &reason);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        bc_log_debugf("Proxy service '%s' got TargetRemoved from manager: %s",
                      proxy->local_service_name,
                      reason);

        /* See above in state_changed for details */

        if (proxy->request_message != NULL && streq(reason, "real")) {
                proxy_service_initial_state_reached(proxy, false);
        }

        return sd_bus_reply_method_return(m, "");
}

/* This is called by the manager when there was an error setting up the monitor.
 * Note, this is only sent once, and if it is sent, expect no other messages.
 */

static int proxy_service_method_error(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_proxy_service_ ProxyService *proxy = proxy_service_ref((ProxyService *) userdata);
        Agent *agent = proxy->agent;

        if (agent == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        const char *message = NULL;
        int r = sd_bus_message_read(m, "s", &message);
        if (r < 0) {
                message = "Got no message";
        }

        bc_log_errorf("Got proxy start error: %s", message);

        proxy_service_initial_state_reached(proxy, false);

        return sd_bus_reply_method_return(m, "");
}

ProxyService *proxy_service_new(
                Agent *agent,
                const char *local_service_name,
                const char *node,
                const char *unit,
                sd_bus_message *request_message) {
        static uint32_t next_id = 0;

        _cleanup_proxy_service_ ProxyService *proxy = malloc0(sizeof(ProxyService));
        if (proxy == NULL) {
                return NULL;
        }

        proxy->ref_count = 1;
        proxy->id = ++next_id;
        proxy->agent = agent;
        proxy->request_message = sd_bus_message_ref(request_message);
        LIST_INIT(proxy_services, proxy);

        proxy->node_name = strdup(node);
        if (proxy->node_name == NULL) {
                return NULL;
        }
        proxy->unit_name = strdup(unit);
        if (proxy->unit_name == NULL) {
                return NULL;
        }
        proxy->local_service_name = strdup(local_service_name);
        if (proxy->local_service_name == NULL) {
                return NULL;
        }

        int r = asprintf(&proxy->object_path, "%s/%u", INTERNAL_PROXY_OBJECT_PATH_PREFIX, proxy->id);
        if (r < 0) {
                return NULL;
        }

        return steal_pointer(&proxy);
}

ProxyService *proxy_service_ref(ProxyService *proxy) {
        proxy->ref_count++;
        return proxy;
}

void proxy_service_unref(ProxyService *proxy) {
        proxy->ref_count--;
        if (proxy->ref_count != 0) {
                return;
        }

        /* Should have been removed from list by now */
        assert(proxy->proxy_services_next == NULL);
        assert(proxy->proxy_services_prev == NULL);

        sd_bus_message_unrefp(&proxy->request_message);
        sd_bus_slot_unrefp(&proxy->export_slot);

        free(proxy->node_name);
        free(proxy->unit_name);
        free(proxy->local_service_name);
        free(proxy->object_path);
        free(proxy);
}

bool proxy_service_export(ProxyService *proxy) {
        int r = sd_bus_add_object_vtable(
                        proxy->agent->peer_dbus,
                        &proxy->export_slot,
                        proxy->object_path,
                        INTERNAL_PROXY_INTERFACE,
                        proxy_service_vtable,
                        proxy);
        if (r < 0) {
                bc_log_errorf("Failed to add service proxy vtable: %s", strerror(-r));
                return false;
        }
        return true;
}

void proxy_service_unexport(ProxyService *proxy) {
        sd_bus_slot_unrefp(&proxy->export_slot);
        proxy->export_slot = NULL;
}

int proxy_service_emit_proxy_new(ProxyService *proxy) {
        Agent *agent = proxy->agent;

        if (!agent_is_connected(agent)) {
                return -ENOTCONN;
        }

        bc_log_infof("Registering new proxy for %s (on %s)", proxy->unit_name, proxy->node_name);
        int res = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "ProxyNew",
                        "sso",
                        proxy->node_name,
                        proxy->unit_name,
                        proxy->object_path);
        if (res >= 0) {
                proxy->sent_new_proxy = true;
        }
        return res;
}

int proxy_service_emit_proxy_removed(ProxyService *proxy) {
        Agent *agent = proxy->agent;

        /* No need to tell the manager if we didn't announce this */
        if (!proxy->sent_new_proxy) {
                return 0;
        }

        if (!agent_is_connected(agent)) {
                return -ENOTCONN;
        }

        bc_log_infof("Unregistering proxy for %s (on %s)", proxy->unit_name, proxy->node_name);
        return sd_bus_emit_signal(
                        proxy->agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "ProxyRemoved",
                        "ss",
                        proxy->node_name,
                        proxy->unit_name);
}
