/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "libbluechi/bus/bus.h"
#include "libbluechi/bus/utils.h"
#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"
#include "libbluechi/common/event-util.h"
#include "libbluechi/common/network.h"
#include "libbluechi/common/opt.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/common/procfs-util.h"
#include "libbluechi/common/time-util.h"
#include "libbluechi/log/log.h"
#include "libbluechi/service/shutdown.h"

#include "agent.h"
#include "proxy.h"

#ifdef USE_USER_API_BUS
#        define ALWAYS_USER_API_BUS 1
#else
#        define ALWAYS_USER_API_BUS 0
#endif

#define DEBUG_SYSTEMD_MESSAGES 0
#define DEBUG_SYSTEMD_MESSAGES_CONTENT 0

typedef struct AgentUnitInfoKey {
        char *object_path;
} AgentUnitInfoKey;

struct JobTracker {
        char *job_object_path;
        job_tracker_callback callback;
        void *userdata;
        free_func_t free_userdata;
        LIST_FIELDS(JobTracker, tracked_jobs);
};

static bool
                agent_track_job(Agent *agent,
                                const char *job_object_path,
                                job_tracker_callback callback,
                                void *userdata,
                                free_func_t free_userdata);

/* Keep track of outstanding systemd job and connect it back to
   the originating bluechi job id so we can proxy changes to it. */
typedef struct {
        int ref_count;

        Agent *agent;
        uint32_t bc_job_id;
        uint64_t job_start_micros;
        char *unit;
        char *method;
} AgentJobOp;

static AgentJobOp *agent_job_op_ref(AgentJobOp *op) {
        op->ref_count++;
        return op;
}

static void agent_job_op_unref(AgentJobOp *op) {
        op->ref_count--;
        if (op->ref_count != 0) {
                return;
        }

        agent_unref(op->agent);
        free(op->unit);
        free(op->method);
        free(op);
}

DEFINE_CLEANUP_FUNC(AgentJobOp, agent_job_op_unref)
#define _cleanup_agent_job_op_ _cleanup_(agent_job_op_unrefp)

static AgentJobOp *agent_job_new(Agent *agent, uint32_t bc_job_id, const char *unit, const char *method) {
        AgentJobOp *op = malloc0(sizeof(AgentJobOp));
        if (op) {
                op->ref_count = 1;
                op->agent = agent_ref(agent);
                op->bc_job_id = bc_job_id;
                op->job_start_micros = 0;
                op->unit = strdup(unit);
                op->method = strdup(method);
        }
        return op;
}

bool agent_is_connected(Agent *agent) {
        return agent != NULL && agent->connection_state == AGENT_CONNECTION_STATE_CONNECTED;
}

char *agent_is_online(Agent *agent) {
        if (agent_is_connected(agent)) {
                return "online";
        }
        return "offline";
}

static int agent_stop_local_proxy_service(Agent *agent, ProxyService *proxy);
static bool agent_connect(Agent *agent);
static bool agent_reconnect(Agent *agent);

static int agent_disconnected(UNUSED sd_bus_message *message, void *userdata, UNUSED sd_bus_error *error) {
        Agent *agent = (Agent *) userdata;

        bc_log_error("Disconnected from manager");

        agent->connection_state = AGENT_CONNECTION_STATE_RETRY;
        int r = sd_bus_emit_properties_changed(
                        agent->api_bus, BC_AGENT_OBJECT_PATH, AGENT_INTERFACE, "Status", NULL);
        if (r < 0) {
                bc_log_errorf("Failed to emit status property changed: %s", strerror(-r));
        }
        r = get_time_seconds(&agent->disconnect_timestamp);
        if (r < 0) {
                bc_log_errorf("Failed to get current time on agent disconnect: %s", strerror(-r));
        }

        /* try to reconnect right away */
        agent_reconnect(agent);

        return 0;
}

static int agent_reset_heartbeat_timer(Agent *agent, sd_event_source **event_source);

static int agent_heartbeat_timer_callback(sd_event_source *event_source, UNUSED uint64_t usec, void *userdata) {
        Agent *agent = (Agent *) userdata;

        int r = 0;
        if (agent->connection_state == AGENT_CONNECTION_STATE_CONNECTED) {
                r = sd_bus_emit_signal(
                                agent->peer_dbus,
                                INTERNAL_AGENT_OBJECT_PATH,
                                INTERNAL_AGENT_INTERFACE,
                                "Heartbeat",
                                "");
                if (r < 0) {
                        bc_log_errorf("Failed to emit heartbeat signal: %s", strerror(-r));
                }
        } else if (agent->connection_state == AGENT_CONNECTION_STATE_RETRY) {
                agent->connection_retry_count++;
                bc_log_infof("Trying to connect to manager (try %d)", agent->connection_retry_count);
                if (!agent_reconnect(agent)) {
                        bc_log_debugf("Connection retry %d failed", agent->connection_retry_count);
                }
        }

        r = agent_reset_heartbeat_timer(agent, &event_source);
        if (r < 0) {
                bc_log_errorf("Failed to reset agent heartbeat timer: %s", strerror(-r));
                return r;
        }

        return 0;
}

static int agent_reset_heartbeat_timer(Agent *agent, sd_event_source **event_source) {
        return event_reset_time_relative(
                        agent->event,
                        event_source,
                        CLOCK_BOOTTIME,
                        agent->heartbeat_interval_msec * USEC_PER_MSEC,
                        0,
                        agent_heartbeat_timer_callback,
                        agent,
                        0,
                        "agent-heartbeat-timer-source",
                        false);
}

static int agent_setup_heartbeat_timer(Agent *agent) {
        _cleanup_(sd_event_source_unrefp) sd_event_source *event_source = NULL;
        int r = 0;

        assert(agent);

        if (agent->heartbeat_interval_msec <= 0) {
                bc_log_warnf("Heartbeat disabled since interval is '%d', ", agent->heartbeat_interval_msec);
                return 0;
        }

        r = agent_reset_heartbeat_timer(agent, &event_source);
        if (r < 0) {
                bc_log_errorf("Failed to reset agent heartbeat timer: %s", strerror(-r));
                return r;
        }

        return sd_event_source_set_floating(event_source, true);
}

SystemdRequest *systemd_request_ref(SystemdRequest *req) {
        req->ref_count++;
        return req;
}

void systemd_request_unref(SystemdRequest *req) {
        Agent *agent = req->agent;

        req->ref_count--;
        if (req->ref_count != 0) {
                return;
        }

        if (req->userdata && req->free_userdata) {
                req->free_userdata(req->userdata);
        }

        sd_bus_message_unrefp(&req->request_message);
        sd_bus_slot_unrefp(&req->slot);
        sd_bus_message_unrefp(&req->message);

        LIST_REMOVE(outstanding_requests, agent->outstanding_requests, req);
        agent_unref(req->agent);
        free(req);
}

static SystemdRequest *agent_create_request_full(
                Agent *agent,
                sd_bus_message *request_message,
                const char *object_path,
                const char *iface,
                const char *method) {
        _cleanup_systemd_request_ SystemdRequest *req = malloc0(sizeof(SystemdRequest));
        if (req == NULL) {
                return NULL;
        }

        req->ref_count = 1;
        req->agent = agent_ref(agent);
        LIST_INIT(outstanding_requests, req);
        req->request_message = sd_bus_message_ref(request_message);

        LIST_APPEND(outstanding_requests, agent->outstanding_requests, req);

        int r = sd_bus_message_new_method_call(
                        agent->systemd_dbus, &req->message, SYSTEMD_BUS_NAME, object_path, iface, method);
        if (r < 0) {
                bc_log_errorf("Failed to create new bus message: %s", strerror(-r));
                return NULL;
        }

        return steal_pointer(&req);
}

static SystemdRequest *agent_create_request(Agent *agent, sd_bus_message *request_message, const char *method) {
        return agent_create_request_full(
                        agent, request_message, SYSTEMD_OBJECT_PATH, SYSTEMD_MANAGER_IFACE, method);
}

static void systemd_request_set_userdata(SystemdRequest *req, void *userdata, free_func_t free_userdata) {
        req->userdata = userdata;
        req->free_userdata = free_userdata;
}

static bool systemd_request_start(SystemdRequest *req, sd_bus_message_handler_t callback) {
        Agent *agent = req->agent;
        AgentJobOp *op = req->userdata;
        if (op != NULL) {
                op->job_start_micros = get_time_micros();
        }

        int r = sd_bus_call_async(
                        agent->systemd_dbus, &req->slot, req->message, callback, req, BC_DEFAULT_DBUS_TIMEOUT);
        if (r < 0) {
                bc_log_errorf("Failed to call async: %s", strerror(-r));
                return false;
        }

        systemd_request_ref(req); /* outstanding callback owns this ref */
        return true;
}

static char *make_unit_path(const char *unit) {
        _cleanup_free_ char *escaped = bus_path_escape(unit);

        if (escaped == NULL) {
                return NULL;
        }

        return strcat_dup(SYSTEMD_OBJECT_PATH "/unit/", escaped);
}

static void unit_info_clear(void *item) {
        AgentUnitInfo *info = item;
        free(info->object_path);
        free(info->unit);
        free(info->substate);
}

static uint64_t unit_info_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const AgentUnitInfo *info = item;
        return hashmap_sip(info->object_path, strlen(info->object_path), seed0, seed1);
}

static int unit_info_compare(const void *a, const void *b, UNUSED void *udata) {
        const AgentUnitInfo *info_a = a;
        const AgentUnitInfo *info_b = b;

        return strcmp(info_a->object_path, info_b->object_path);
}

static const char *unit_info_get_substate(AgentUnitInfo *info) {
        return info->substate ? info->substate : "invalid";
}

struct UpdateStateData {
        UnitActiveState active_state;
        const char *substate;
};


static int unit_info_update_state_cb(const char *key, const char *value_type, sd_bus_message *m, void *userdata) {
        struct UpdateStateData *data = userdata;
        const char *value = NULL;

        if (!streq(key, "ActiveState") && !streq(key, "SubState") && !streq(value_type, "s")) {
                return 2; /* skip */
        }

        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, value_type);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &value);
        if (r < 0) {
                return r;
        }

        if (streq(key, "ActiveState")) {
                data->active_state = active_state_from_string(value);
        } else if (streq(key, "SubState")) {
                data->substate = value;
        }

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                return r;
        }

        return 0;
}


/* Updates stored unit state from property change, returns true if the state changes */
static bool unit_info_update_state(AgentUnitInfo *info, sd_bus_message *m) {
        struct UpdateStateData data = {
                info->active_state,
                unit_info_get_substate(info),
        };
        int r = bus_parse_properties_foreach(m, unit_info_update_state_cb, &data);
        if (r < 0) {
                return false;
        }

        bool changed = false;
        if (data.active_state != info->active_state) {
                info->active_state = data.active_state;
                changed = true;
        }
        if (!streq(data.substate, unit_info_get_substate(info))) {
                free(info->substate);
                info->substate = strdup(data.substate);
                changed = true;
        }

        return changed;
}

Agent *agent_new(void) {
        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                bc_log_errorf("Failed to create event loop: %s", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = strdup(BC_AGENT_DBUS_NAME);
        if (service_name == NULL) {
                bc_log_error("Out of memory");
                return NULL;
        }

        _cleanup_agent_ Agent *agent = malloc0(sizeof(Agent));
        agent->ref_count = 1;
        agent->event = steal_pointer(&event);
        agent->api_bus_service_name = steal_pointer(&service_name);
        LIST_HEAD_INIT(agent->outstanding_requests);
        LIST_HEAD_INIT(agent->tracked_jobs);
        LIST_HEAD_INIT(agent->proxy_services);

        agent->unit_infos = hashmap_new(
                        sizeof(AgentUnitInfo), 0, 0, 0, unit_info_hash, unit_info_compare, unit_info_clear, NULL);
        if (agent->unit_infos == NULL) {
                return NULL;
        }

        agent->connection_state = AGENT_CONNECTION_STATE_DISCONNECTED;
        agent->connection_retry_count = 0;
        agent->wildcard_subscription_active = false;
        agent->metrics_enabled = false;
        agent->disconnect_timestamp = 0;

        return steal_pointer(&agent);
}

Agent *agent_ref(Agent *agent) {
        assert(agent->ref_count > 0);

        agent->ref_count++;
        return agent;
}

/* Emit == false on agent shutdown or proxy creation error */
void agent_remove_proxy(Agent *agent, ProxyService *proxy, bool emit_removed) {
        assert(proxy->agent == agent);

        bc_log_debugf("Removing proxy %s %s", proxy->node_name, proxy->unit_name);

        if (emit_removed) {
                int r = proxy_service_emit_proxy_removed(proxy);
                if (r < 0) {
                        bc_log_errorf("Failed to emit ProxyRemoved signal for node %s and unit %s",
                                      proxy->node_name,
                                      proxy->unit_name);
                }
        }

        if (proxy->request_message != NULL) {
                int r = sd_bus_reply_method_errorf(
                                proxy->request_message, SD_BUS_ERROR_FAILED, "Proxy service stopped");
                if (r < 0) {
                        bc_log_errorf("Failed to sent ready status message to proxy: %s", strerror(-r));
                }
                sd_bus_message_unrefp(&proxy->request_message);
                proxy->request_message = NULL;
        }


        if (proxy->sent_successful_ready) {
                int r = agent_stop_local_proxy_service(agent, proxy);
                if (r < 0) {
                        bc_log_errorf("Failed to stop proxy service: %s", strerror(-r));
                }
        }

        proxy_service_unexport(proxy);

        LIST_REMOVE(proxy_services, agent->proxy_services, proxy);
        proxy->agent = NULL;
        proxy_service_unref(proxy);
}

void agent_unref(Agent *agent) {
        assert(agent->ref_count > 0);

        agent->ref_count--;
        if (agent->ref_count != 0) {
                return;
        }

        bc_log_debug("Finalizing agent");

        /* These are removed in agent_stop */
        assert(agent->proxy_services == NULL);

        hashmap_free(agent->unit_infos);

        free(agent->name);
        free(agent->host);
        free(agent->orch_addr);
        free(agent->api_bus_service_name);

        if (agent->event != NULL) {
                sd_event_unrefp(&agent->event);
        }

        if (agent->metrics_slot != NULL) {
                sd_bus_slot_unrefp(&agent->metrics_slot);
        }

        if (agent->peer_dbus != NULL) {
                sd_bus_unrefp(&agent->peer_dbus);
        }
        if (agent->api_bus != NULL) {
                sd_bus_unrefp(&agent->api_bus);
        }
        if (agent->systemd_dbus != NULL) {
                sd_bus_unrefp(&agent->systemd_dbus);
        }

        if (agent->config) {
                cfg_dispose(agent->config);
                agent->config = NULL;
        }
        free(agent);
}

bool agent_set_port(Agent *agent, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                bc_log_errorf("Invalid port format '%s'", port_s);
                return false;
        }
        agent->port = port;
        return true;
}

bool agent_set_orch_address(Agent *agent, const char *address) {
        return copy_str(&agent->orch_addr, address);
}

bool agent_set_host(Agent *agent, const char *host) {
        return copy_str(&agent->host, host);
}

bool agent_set_name(Agent *agent, const char *name) {
        return copy_str(&agent->name, name);
}

bool agent_set_heartbeat_interval(Agent *agent, const char *interval_msec) {
        long interval = 0;

        if (!parse_long(interval_msec, &interval)) {
                bc_log_errorf("Invalid heartbeat interval format '%s'", interval_msec);
                return false;
        }
        agent->heartbeat_interval_msec = interval;
        return true;
}


void agent_set_systemd_user(Agent *agent, bool systemd_user) {
        agent->systemd_user = systemd_user;
}

bool agent_parse_config(Agent *agent, const char *configfile) {
        int result = 0;

        result = cfg_initialize(&agent->config);
        if (result != 0) {
                fprintf(stderr, "Error initializing configuration: '%s'.\n", strerror(-result));
                return false;
        }

        result = cfg_agent_def_conf(agent->config);
        if (result < 0) {
                fprintf(stderr, "Failed to set default settings for agent: %s", strerror(-result));
                return false;
        }
        result = cfg_load_complete_configuration(
                        agent->config, CFG_AGENT_DEFAULT_CONFIG, CFG_ETC_BC_AGENT_CONF, CFG_ETC_AGENT_CONF_DIR);
        if (result != 0) {
                return false;
        }

        if (configfile != NULL) {
                result = cfg_load_from_file(agent->config, configfile);
                if (result < 0) {
                        fprintf(stderr,
                                "Error loading configuration file '%s', error code '%s'.\n",
                                configfile,
                                strerror(-result));
                        return false;
                } else if (result > 0) {
                        fprintf(stderr, "Error parsing configuration file '%s' on line %d\n", configfile, result);
                        return false;
                }
        }


        // set logging configuration
        bc_log_init(agent->config);

        const char *value = NULL;
        value = cfg_get_value(agent->config, CFG_NODE_NAME);
        if (value) {
                if (!agent_set_name(agent, value)) {
                        return false;
                }
        }

        value = cfg_get_value(agent->config, CFG_MANAGER_HOST);
        if (value) {
                if (!agent_set_host(agent, value)) {
                        return false;
                }
        }

        value = cfg_get_value(agent->config, CFG_MANAGER_PORT);
        if (value) {
                if (!agent_set_port(agent, value)) {
                        return false;
                }
        }

        value = cfg_get_value(agent->config, CFG_MANAGER_ADDRESS);
        if (value) {
                if (!agent_set_orch_address(agent, value)) {
                        return false;
                }
        }

        value = cfg_get_value(agent->config, CFG_HEARTBEAT_INTERVAL);
        if (value) {
                if (!agent_set_heartbeat_interval(agent, value)) {
                        return false;
                }
        }

        _cleanup_free_ const char *dumped_cfg = cfg_dump(agent->config);
        bc_log_debug_with_data("Final configuration used", "\n%s", dumped_cfg);

        return true;
}

static void agent_update_unit_infos_for(Agent *agent, AgentUnitInfo *info) {
        if (!info->subscribed && !info->loaded) {
                AgentUnitInfoKey key = { info->object_path };
                AgentUnitInfo *info = hashmap_delete(agent->unit_infos, &key);
                if (info) {
                        unit_info_clear(info);
                }
        }
}

static AgentUnitInfo *agent_get_unit_info(Agent *agent, const char *unit_path) {
        AgentUnitInfoKey key = { (char *) unit_path };

        return hashmap_get(agent->unit_infos, &key);
}

static AgentUnitInfo *agent_ensure_unit_info(Agent *agent, const char *unit) {
        _cleanup_free_ char *unit_path = make_unit_path(unit);
        AgentUnitInfo *info = agent_get_unit_info(agent, unit_path);
        if (info != NULL) {
                return info;
        }

        _cleanup_free_ char *unit_copy = strdup(unit);
        if (unit_copy == NULL) {
                return NULL;
        }

        AgentUnitInfo v = { unit_path, unit_copy, false, false, -1, NULL };

        AgentUnitInfo *replaced = hashmap_set(agent->unit_infos, &v);
        if (replaced == NULL && hashmap_oom(agent->unit_infos)) {
                return NULL;
        }

        info = agent_get_unit_info(agent, unit_path);

        /* These are now in hashtable */
        steal_pointer(&unit_copy);
        steal_pointer(&unit_path);

        return info;
}

static int agent_method_passthrough_to_systemd_cb(
                sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_passthrough_to_systemd(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(
                        agent, m, sd_bus_message_get_member(m));
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        int r = sd_bus_message_copy(req->message, m, true);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, agent_method_passthrough_to_systemd_cb)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.ListUnits **
 ************************************************************************/

static int list_units_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;

        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_list_units(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, m, "ListUnits");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, list_units_callback)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/************************************************************************
 ******** org.eclipse.bluechi.internal.Agent.GetUnitProperties ************
 ************************************************************************/

static int get_unit_properties_got_properties(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_get_unit_properties(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *interface = NULL;
        const char *unit = NULL;
        _cleanup_free_ char *unit_path = NULL;

        int r = sd_bus_message_read(m, "ss", &unit, &interface);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        r = assemble_object_path_string(SYSTEMD_OBJECT_PATH "/unit", unit, &unit_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request_full(
                        agent, m, unit_path, "org.freedesktop.DBus.Properties", "GetAll");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "s", interface);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, get_unit_properties_got_properties)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/***************************************************************************
 ******** org.eclipse.bluechi.internal.Agent.GetUnitProperty *
 ***************************************************************************/

static int get_unit_property_got_property(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_get_unit_property(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *interface = NULL;
        const char *unit = NULL;
        const char *property = NULL;
        _cleanup_free_ char *unit_path = NULL;

        int r = sd_bus_message_read(m, "sss", &unit, &interface, &property);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        r = assemble_object_path_string(SYSTEMD_OBJECT_PATH "/unit", unit, &unit_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request_full(
                        agent, m, unit_path, "org.freedesktop.DBus.Properties", "Get");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "ss", interface, property);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, get_unit_property_got_property)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/******************************************************************************
 ******** org.eclipse.bluechi.internal.Agent.SetUnitProperties  *
 ******************************************************************************/

static int set_unit_properties_cb(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_set_unit_properties(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        int runtime = 0;
        _cleanup_free_ char *unit_path = NULL;

        int r = sd_bus_message_read(m, "sb", &unit, &runtime);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        r = assemble_object_path_string(SYSTEMD_OBJECT_PATH "/unit", unit, &unit_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request_full(
                        agent, m, unit_path, "org.freedesktop.systemd1.Unit", "SetProperties");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "b", runtime);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_copy(req->message, m, false);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, set_unit_properties_cb)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/******************************************************************************
 ******** org.eclipse.bluechi.internal.Agent.FreezeUnit  *
 ******************************************************************************/

static int freeze_unit_cb(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_freeze_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        _cleanup_free_ char *unit_path = NULL;

        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        r = assemble_object_path_string(SYSTEMD_OBJECT_PATH "/unit", unit, &unit_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request_full(
                        agent, m, unit_path, "org.freedesktop.systemd1.Unit", "Freeze");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, freeze_unit_cb)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/******************************************************************************
 ******** org.eclipse.bluechi.internal.Agent.ThawUnit  *
 ******************************************************************************/

static int thaw_unit_cb(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        int r = sd_bus_message_new_method_return(req->request_message, &reply);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(reply, m, true);
        if (r < 0) {
                return r;
        }

        return sd_bus_message_send(reply);
}

static int agent_method_thaw_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        _cleanup_free_ char *unit_path = NULL;

        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        r = assemble_object_path_string(SYSTEMD_OBJECT_PATH "/unit", unit, &unit_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request_full(
                        agent, m, unit_path, "org.freedesktop.systemd1.Unit", "Thaw");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, thaw_unit_cb)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

static void agent_job_done(UNUSED sd_bus_message *m, const char *result, void *userdata) {
        AgentJobOp *op = userdata;
        Agent *agent = op->agent;

        if (agent->metrics_enabled) {
                agent_send_job_metrics(
                                agent,
                                op->unit,
                                op->method,
                                // NOLINTNEXTLINE(bugprone-narrowing-conversions, cppcoreguidelines-narrowing-conversions)
                                finalize_time_interval_micros(op->job_start_micros));
        }

        bc_log_infof("Sending JobDone %u, result: %s", op->bc_job_id, result);

        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "JobDone",
                        "us",
                        op->bc_job_id,
                        result);
        if (r < 0) {
                bc_log_errorf("Failed to emit JobDone: %s", strerror(-r));
        }
}

static int unit_lifecycle_method_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;
        Agent *agent = req->agent;
        const char *job_object_path = NULL;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        int r = sd_bus_message_read(m, "o", &job_object_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(req->request_message, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        AgentJobOp *op = req->userdata;

        if (!agent_track_job(
                            agent,
                            job_object_path,
                            agent_job_done,
                            agent_job_op_ref(op),
                            (free_func_t) agent_job_op_unref)) {
                return sd_bus_reply_method_errorf(req->request_message, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        return sd_bus_reply_method_return(req->request_message, "");
}

static int agent_run_unit_lifecycle_method(sd_bus_message *m, Agent *agent, const char *method) {
        const char *name = NULL;
        const char *mode = NULL;
        uint32_t job_id = 0;

        int r = sd_bus_message_read(m, "ssu", &name, &mode, &job_id);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        bc_log_infof("Request to %s unit: %s - Action: %s", method, name, mode);

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, m, method);
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        _cleanup_agent_job_op_ AgentJobOp *op = agent_job_new(agent, job_id, name, method);
        if (op == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        systemd_request_set_userdata(req, agent_job_op_ref(op), (free_func_t) agent_job_op_unref);

        r = sd_bus_message_append(req->message, "ss", name, mode);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, unit_lifecycle_method_callback)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/*************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.StartUnit   *
 *************************************************************************/

static int agent_method_start_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "StartUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.StopUnit    *
 *************************************************************************/

static int agent_method_stop_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "StopUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.RestartUnit *
 *************************************************************************/

static int agent_method_restart_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "RestartUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.ReloadUnit  *
 *************************************************************************/

static int agent_method_reload_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "ReloadUnit");
}

/*************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.Subscribe ***
 *************************************************************************/

static void agent_emit_unit_new(Agent *agent, AgentUnitInfo *info, const char *reason) {
        bc_log_debugf("Sending UnitNew %s, reason: %s", info->unit, reason);
        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "UnitNew",
                        "ss",
                        info->unit,
                        reason);
        if (r < 0) {
                bc_log_warn("Failed to emit UnitNew");
        }
}

static void agent_emit_unit_removed(Agent *agent, AgentUnitInfo *info) {
        bc_log_debugf("Sending UnitRemoved %s", info->unit);

        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "UnitRemoved",
                        "s",
                        info->unit);
        if (r < 0) {
                bc_log_warn("Failed to emit UnitRemoved");
        }
}

static void agent_emit_unit_state_changed(Agent *agent, AgentUnitInfo *info, const char *reason) {
        bc_log_debugf("Sending UnitStateChanged %s %s(%s) reason: %s",
                      info->unit,
                      active_state_to_string(info->active_state),
                      unit_info_get_substate(info),
                      reason);
        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "UnitStateChanged",
                        "ssss",
                        info->unit,
                        active_state_to_string(info->active_state),
                        unit_info_get_substate(info),
                        reason);
        if (r < 0) {
                bc_log_warn("Failed to emit UnitStateChanged");
        }
}

static int agent_method_subscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        if (is_wildcard(unit)) {
                if (agent->wildcard_subscription_active) {
                        return sd_bus_reply_method_errorf(
                                        m, SD_BUS_ERROR_FAILED, "Already wildcard subscribed");
                }
                agent->wildcard_subscription_active = true;

                AgentUnitInfo info = { NULL, (char *) unit, true, true, -1, NULL };
                agent_emit_unit_new(agent, &info, "virtual");

                return sd_bus_reply_method_return(m, "");
        }

        AgentUnitInfo *info = agent_ensure_unit_info(agent, unit);
        if (info == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (info->subscribed) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Already subscribed");
        }

        info->subscribed = true;

        if (info->loaded) {
                /* The unit was already loaded, synthesize a UnitNew */
                agent_emit_unit_new(agent, info, "virtual");

                if (info->active_state != _UNIT_ACTIVE_STATE_INVALID) {
                        agent_emit_unit_state_changed(agent, info, "virtual");
                }
        }

        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 *** org.eclipse.bluechi.internal.Agent.Unsubscribe ********
 *************************************************************************/

static int agent_method_unsubscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        if (is_wildcard(unit)) {
                if (!agent->wildcard_subscription_active) {
                        return sd_bus_reply_method_errorf(
                                        m, SD_BUS_ERROR_FAILED, "No wildcard subscription active");
                }
                agent->wildcard_subscription_active = false;
                return sd_bus_reply_method_return(m, "");
        }

        _cleanup_free_ char *path = make_unit_path(unit);
        if (path == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        AgentUnitInfo *info = agent_get_unit_info(agent, path);
        if (info == NULL || !info->subscribed) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Not subscribed");
        }

        info->subscribed = false;
        agent_update_unit_infos_for(agent, info);
        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 ********** org.eclipse.bluechi.internal.Agent.StartDep ****
 *************************************************************************/

static char *get_dep_unit(const char *unit_name) {
        return strcat_dup("bluechi-dep@", unit_name);
}

static int start_dep_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Error starting dep service: %s", sd_bus_message_get_error(m)->message);
        }
        return 0;
}

static int agent_method_start_dep(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        _cleanup_free_ char *dep_unit = get_dep_unit(unit);
        if (dep_unit == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        bc_log_infof("Starting dependency %s", dep_unit);

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, m, "StartUnit");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "ss", dep_unit, "replace");
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, start_dep_callback)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 0;
}

/*************************************************************************
 **** org.eclipse.bluechi.internal.Agent.StopDep ***********
 *************************************************************************/

static int stop_dep_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Error stopping dep service: %s", sd_bus_message_get_error(m)->message);
        }
        return 0;
}

static int agent_method_stop_dep(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        _cleanup_free_ char *dep_unit = get_dep_unit(unit);
        if (dep_unit == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        bc_log_infof("Stopping dependency %s", dep_unit);

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, m, "StopUnit");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "ss", dep_unit, "replace");
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, stop_dep_callback)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 0;
}


/***************************************************************************
 **** org.eclipse.bluechi.internal.Agent.EnableMetrics *******
 ***************************************************************************/

static const sd_bus_vtable agent_metrics_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "AgentJobMetrics",
                        "sst",
                        SD_BUS_PARAM(unit) SD_BUS_PARAM(method) SD_BUS_PARAM(systemd_job_time_micros),
                        0),
        SD_BUS_VTABLE_END
};

static int agent_method_enable_metrics(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        int r = 0;

        if (agent->metrics_enabled) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Metrics already enabled");
        }
        agent->metrics_enabled = true;
        r = sd_bus_add_object_vtable(
                        agent->peer_dbus,
                        &agent->metrics_slot,
                        INTERNAL_AGENT_METRICS_OBJECT_PATH,
                        INTERNAL_AGENT_METRICS_INTERFACE,
                        agent_metrics_vtable,
                        NULL);
        if (r < 0) {
                bc_log_errorf("Failed to add metrics vtable: %s", strerror(-r));
                return r;
        }
        bc_log_debug("Metrics enabled");
        return sd_bus_reply_method_return(m, "");
}

/***************************************************************************
 **** org.eclipse.bluechi.internal.Agent.DisableMetrics ******
 ***************************************************************************/

static int agent_method_disable_metrics(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;

        if (!agent->metrics_enabled) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Metrics already disabled");
        }
        agent->metrics_enabled = false;
        sd_bus_slot_unrefp(&agent->metrics_slot);
        bc_log_debug("Metrics disabled");
        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 ************** org.eclipse.bluechi.Agent.SetNodeLogLevel  *
 *************************************************************************/

static int agent_method_set_log_level(
                UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *level = NULL;
        int r = sd_bus_message_read(m, "s", &level);
        if (r < 0) {
                bc_log_errorf("Failed to read Loglevel parameter: %s", strerror(-r));
                return sd_bus_reply_method_errorf(
                                m, SD_BUS_ERROR_FAILED, "Failed to read Loglevel parameter: %s", strerror(-r));
        }
        LogLevel loglevel = string_to_log_level(level);
        if (loglevel == LOG_LEVEL_INVALID) {
                bc_log_errorf("Invalid input for log level: %s", loglevel);
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Invalid input for log level");
        }
        bc_log_set_level(loglevel);
        bc_log_infof("Log level changed to %s", level);
        return sd_bus_reply_method_return(m, "");
}

/*************************************************************************
 ************** org.eclipse.bluechi.Agent.GetSystemResources     *
 *************************************************************************/

static int agent_method_get_system_resources(
                UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        uint32_t cpu_number = 0;
        uint64_t cpu_time = 0;
        uint64_t memory_total = 0;
        uint64_t memory_used = 0;
        long ret = 0;

        int r = sd_bus_message_new_method_return(m, &reply);
        if (r < 0) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }

        ret = sysconf(_SC_NPROCESSORS_ONLN);
        if (ret < 0) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }
        assert(ret > 0);
        cpu_number = ret;

        r = procfs_cpu_get_usage(&cpu_time);
        if (r < 0) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = procfs_memory_get(&memory_total, &memory_used);
        if (r < 0) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(reply, "uttt", cpu_number, cpu_time, memory_total, memory_used);
        if (r < 0) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return sd_bus_message_send(reply);
}


static const sd_bus_vtable internal_agent_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", UNIT_INFO_STRUCT_ARRAY_TYPESTRING, agent_method_list_units, 0),
        SD_BUS_METHOD("GetUnitProperties", "ss", "a{sv}", agent_method_get_unit_properties, 0),
        SD_BUS_METHOD("GetUnitProperty", "sss", "v", agent_method_get_unit_property, 0),
        SD_BUS_METHOD("SetUnitProperties", "sba(sv)", "", agent_method_set_unit_properties, 0),
        SD_BUS_METHOD("StartUnit", "ssu", "", agent_method_start_unit, 0),
        SD_BUS_METHOD("StopUnit", "ssu", "", agent_method_stop_unit, 0),
        SD_BUS_METHOD("FreezeUnit", "s", "", agent_method_freeze_unit, 0),
        SD_BUS_METHOD("ThawUnit", "s", "", agent_method_thaw_unit, 0),
        SD_BUS_METHOD("RestartUnit", "ssu", "", agent_method_restart_unit, 0),
        SD_BUS_METHOD("ReloadUnit", "ssu", "", agent_method_reload_unit, 0),
        SD_BUS_METHOD("Subscribe", "s", "", agent_method_subscribe, 0),
        SD_BUS_METHOD("Unsubscribe", "s", "", agent_method_unsubscribe, 0),
        SD_BUS_METHOD("EnableMetrics", "", "", agent_method_enable_metrics, 0),
        SD_BUS_METHOD("DisableMetrics", "", "", agent_method_disable_metrics, 0),
        SD_BUS_METHOD("SetLogLevel", "s", "", agent_method_set_log_level, 0),
        SD_BUS_METHOD("GetSystemResources", "", "uttt", agent_method_get_system_resources, 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobDone", "us", SD_BUS_PARAM(id) SD_BUS_PARAM(result), 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobStateChanged", "us", SD_BUS_PARAM(id) SD_BUS_PARAM(state), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitPropertiesChanged",
                        "ssa{sv}",
                        SD_BUS_PARAM(unit) SD_BUS_PARAM(iface) SD_BUS_PARAM(properties),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitNew", "ss", SD_BUS_PARAM(unit) SD_BUS_PARAM(reason), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitStateChanged",
                        "ssss",
                        SD_BUS_PARAM(unit) SD_BUS_PARAM(active_state) SD_BUS_PARAM(substate)
                                        SD_BUS_PARAM(reason),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitRemoved", "s", SD_BUS_PARAM(unit), 0),
        SD_BUS_SIGNAL("Heartbeat", "", 0),
        SD_BUS_METHOD("StartDep", "s", "", agent_method_start_dep, 0),
        SD_BUS_METHOD("StopDep", "s", "", agent_method_stop_dep, 0),
        SD_BUS_METHOD("EnableUnitFiles", "asbb", "ba(sss)", agent_method_passthrough_to_systemd, 0),
        SD_BUS_METHOD("DisableUnitFiles", "asb", "a(sss)", agent_method_passthrough_to_systemd, 0),
        SD_BUS_METHOD("Reload", "", "", agent_method_passthrough_to_systemd, 0),
        SD_BUS_VTABLE_END
};

/*************************************************************************
 ********** org.eclipse.bluechi.Agent.CreateProxy **********
 *************************************************************************/

static ProxyService *agent_find_proxy(
                Agent *agent, const char *local_service_name, const char *node_name, const char *unit_name) {
        ProxyService *proxy = NULL;

        LIST_FOREACH(proxy_services, proxy, agent->proxy_services) {
                if (streq(proxy->local_service_name, local_service_name) &&
                    streq(proxy->node_name, node_name) && streq(proxy->unit_name, unit_name)) {
                        return proxy;
                }
        }

        return NULL;
}

static int agent_method_create_proxy(UNUSED sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = (Agent *) userdata;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;
        const char *local_service_name = NULL;
        const char *node_name = NULL;
        const char *unit_name = NULL;
        int r = sd_bus_message_read(m, "sss", &local_service_name, &node_name, &unit_name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        bc_log_infof("CreateProxy request from %s", local_service_name);

        ProxyService *old_proxy = agent_find_proxy(agent, local_service_name, node_name, unit_name);
        if (old_proxy) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_ADDRESS_IN_USE, "Proxy already exists");
        }

        _cleanup_proxy_service_ ProxyService *proxy = proxy_service_new(
                        agent, local_service_name, node_name, unit_name, m);
        if (proxy == NULL) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!proxy_service_export(proxy)) {
                return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = proxy_service_emit_proxy_new(proxy);
        if (r < 0 && r != -ENOTCONN) {
                bc_log_errorf("Failed to emit ProxyNew signal: %s", strerror(-r));
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        LIST_APPEND(proxy_services, agent->proxy_services, proxy_service_ref(proxy));

        return 1;
}

/*************************************************************************
 **** org.eclipse.bluechi.Agent.RemoveProxy ****************
 *************************************************************************/

/* This is called when the proxy service is shut down, via the
 * `ExecStop=bluechi-proxy remove` command being called. This will
 * happen when no services depends on the proxy anymore, as well as
 * when the agent explicitly tells the proxy to stop.
 */

static int agent_method_remove_proxy(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = (Agent *) userdata;
        const char *local_service_name = NULL;
        const char *node_name = NULL;
        const char *unit_name = NULL;
        int r = sd_bus_message_read(m, "sss", &local_service_name, &node_name, &unit_name);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
        }

        bc_log_infof("RemoveProxy request from %s", local_service_name);

        ProxyService *proxy = agent_find_proxy(agent, local_service_name, node_name, unit_name);
        if (proxy == NULL) {
                /* NOTE: This happens pretty often, when we stopped the proxy service ourselves
                   and removed the proxy, but then the ExecStop triggers, so we sent a success here. */
                return sd_bus_reply_method_return(m, "");
        }

        /* We got this because the proxy is shutting down, no need to manually do it */
        proxy->dont_stop_proxy = true;

        agent_remove_proxy(agent, proxy, true);

        return sd_bus_reply_method_return(m, "");
}


/*************************************************************************
 **** org.eclipse.bluechi.Agent.Status ****************
 *************************************************************************/

static int agent_property_get_status(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;

        return sd_bus_message_append(reply, "s", agent_is_online(agent));
}

/*************************************************************************
 **** org.eclipse.bluechi.Agent.LastSuccessfulHeartbeat ****************
 *************************************************************************/

static int agent_property_get_disconnect_timestamp(
                UNUSED sd_bus *bus,
                UNUSED const char *path,
                UNUSED const char *interface,
                UNUSED const char *property,
                sd_bus_message *reply,
                void *userdata,
                UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;

        return sd_bus_message_append(reply, "t", agent->disconnect_timestamp);
}


static const sd_bus_vtable agent_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("CreateProxy", "sss", "", agent_method_create_proxy, 0),
        SD_BUS_METHOD("RemoveProxy", "sss", "", agent_method_remove_proxy, 0),

        SD_BUS_PROPERTY("Status", "s", agent_property_get_status, 0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
        SD_BUS_PROPERTY("DisconnectTimestamp",
                        "t",
                        agent_property_get_disconnect_timestamp,
                        0,
                        SD_BUS_VTABLE_PROPERTY_EXPLICIT),
        SD_BUS_VTABLE_END
};


static void job_tracker_free(JobTracker *track) {
        if (track->userdata && track->free_userdata) {
                track->free_userdata(track->userdata);
        }
        free(track->job_object_path);
        free(track);
}

static void job_tracker_freep(JobTracker **trackp) {
        if (trackp && *trackp) {
                job_tracker_free(*trackp);
                *trackp = NULL;
        }
}

#define _cleanup_job_tracker_ _cleanup_(job_tracker_freep)

static bool
                agent_track_job(Agent *agent,
                                const char *job_object_path,
                                job_tracker_callback callback,
                                void *userdata,
                                free_func_t free_userdata) {
        _cleanup_job_tracker_ JobTracker *track = malloc0(sizeof(JobTracker));
        if (track == NULL) {
                return false;
        }

        track->job_object_path = strdup(job_object_path);
        if (track->job_object_path == NULL) {
                return false;
        }

        track->callback = callback;
        track->userdata = userdata;
        track->free_userdata = free_userdata;
        LIST_INIT(tracked_jobs, track);

        LIST_PREPEND(tracked_jobs, agent->tracked_jobs, steal_pointer(&track));

        return true;
}


static int agent_match_job_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *interface = NULL;
        const char *state = NULL;
        const char *object_path = sd_bus_message_get_path(m);
        JobTracker *track = NULL;
        AgentJobOp *op = NULL;

        int r = sd_bus_message_read(m, "s", &interface);
        if (r < 0) {
                bc_log_errorf("Failed to read job property: %s", strerror(-r));
                return r;
        }

        /* Only handle Job iface changes */
        if (!streq(interface, "org.freedesktop.systemd1.Job")) {
                return 0;
        }

        /* Look for tracked jobs */
        LIST_FOREACH(tracked_jobs, track, agent->tracked_jobs) {
                if (streq(track->job_object_path, object_path)) {
                        op = track->userdata;
                        break;
                }
        }

        if (op == NULL) {
                return 0;
        }

        r = bus_parse_property_string(m, "State", &state);
        if (r < 0) {
                if (r == -ENOENT) {
                        return 0; /* Some other property changed */
                }
                bc_log_errorf("Failed to get job property: %s", strerror(-r));
                return r;
        }

        r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "JobStateChanged",
                        "us",
                        op->bc_job_id,
                        state);
        if (r < 0) {
                bc_log_errorf("Failed to emit JobStateChanged: %s", strerror(-r));
        }

        return 0;
}

static int agent_match_unit_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *interface = NULL;

        int r = sd_bus_message_read(m, "s", &interface);
        if (r < 0) {
                return r;
        }

        AgentUnitInfo *info = agent_get_unit_info(agent, sd_bus_message_get_path(m));
        if (info == NULL) {
                return 0;
        }

        if (streq(interface, "org.freedesktop.systemd1.Unit")) {
                bool state_changed = unit_info_update_state(info, m);
                if (state_changed && (info->subscribed || agent->wildcard_subscription_active)) {
                        agent_emit_unit_state_changed(agent, info, "real");
                }
        }

        if (!info->subscribed && !agent->wildcard_subscription_active) {
                return 0;
        }

        /* Rewind and skip to copy content again */
        (void) sd_bus_message_rewind(m, true);
        (void) sd_bus_message_skip(m, "s"); // re-skip interface

        bc_log_debugf("Sending UnitPropertiesChanged %s", info->unit);

        /* Forward the property changes */
        _cleanup_sd_bus_message_ sd_bus_message *sig = NULL;
        r = sd_bus_message_new_signal(
                        agent->peer_dbus,
                        &sig,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "UnitPropertiesChanged");
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_append(sig, "s", info->unit);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_append(sig, "s", interface);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_copy(sig, m, false);
        if (r < 0) {
                return r;
        }

        return sd_bus_send(agent->peer_dbus, sig, NULL);
}

static int agent_match_unit_new(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Agent *agent = userdata;
        const char *unit_name = NULL;
        const char *path = NULL;

        int r = sd_bus_message_read(m, "so", &unit_name, &path);
        if (r < 0) {
                return r;
        }

        AgentUnitInfo *info = agent_ensure_unit_info(agent, unit_name);
        if (info == NULL) {
                return 0;
        }

        info->loaded = true;
        /* Systemd services start in inactive(dead) state, so use this as the first
         * state. This way we can detect when the state changed without sporadic
         * changes to "dead".
         */
        info->active_state = UNIT_INACTIVE;
        info->substate = strdup("dead");

        if (info->subscribed || agent->wildcard_subscription_active) {
                /* Forward the event */
                agent_emit_unit_new(agent, info, "real");
        }

        return 0;
}

static int agent_match_unit_removed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Agent *agent = userdata;
        const char *id = NULL;
        const char *path = NULL;

        int r = sd_bus_message_read(m, "so", &id, &path);
        if (r < 0) {
                return r;
        }

        AgentUnitInfo *info = agent_get_unit_info(agent, path);
        if (info == NULL) {
                return 0;
        }

        info->loaded = false;
        info->active_state = _UNIT_ACTIVE_STATE_INVALID;
        free(info->substate);
        info->substate = NULL;

        if (info->subscribed || agent->wildcard_subscription_active) {
                /* Forward the event */
                agent_emit_unit_removed(agent, info);
        }

        /* Maybe remove if unloaded and no other interest in it */
        agent_update_unit_infos_for(agent, info);

        return 1;
}


static int agent_match_job_removed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        Agent *agent = userdata;
        const char *job_path = NULL;
        const char *unit = NULL;
        const char *result = NULL;
        JobTracker *track = NULL, *next_track = NULL;
        uint32_t id = 0;
        int r = 0;

        r = sd_bus_message_read(m, "uoss", &id, &job_path, &unit, &result);
        if (r < 0) {
                bc_log_errorf("Can't parse job result: %s", strerror(-r));
                return r;
        }

        (void) sd_bus_message_rewind(m, true);

        LIST_FOREACH_SAFE(tracked_jobs, track, next_track, agent->tracked_jobs) {
                if (streq(track->job_object_path, job_path)) {
                        LIST_REMOVE(tracked_jobs, agent->tracked_jobs, track);
                        track->callback(m, result, track->userdata);
                        job_tracker_free(track);
                        break;
                }
        }

        return 0;
}

static int debug_systemd_message_handler(
                sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        bc_log_infof("Incoming message from systemd: path: %s, iface: %s, member: %s, signature: '%s'",
                     sd_bus_message_get_path(m),
                     sd_bus_message_get_interface(m),
                     sd_bus_message_get_member(m),
                     sd_bus_message_get_signature(m, true));
        if (DEBUG_SYSTEMD_MESSAGES_CONTENT) {
                sd_bus_message_dump(m, stderr, 0);
                sd_bus_message_rewind(m, true);
        }
        return 0;
}

int agent_init_units(Agent *agent, sd_bus_message *m) {
        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, UNIT_INFO_STRUCT_TYPESTRING);
        if (r < 0) {
                return r;
        }


        while (sd_bus_message_at_end(m, false) == 0) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, UNIT_INFO_TYPESTRING);
                if (r < 0) {
                        return r;
                }

                // NOLINTBEGIN(cppcoreguidelines-init-variables)
                const char *name, *desc, *load_state, *active_state, *sub_state, *follower, *object_path,
                                *job_type, *job_object_path;
                uint32_t job_id;
                // NOLINTEND(cppcoreguidelines-init-variables)

                r = sd_bus_message_read(
                                m,
                                UNIT_INFO_TYPESTRING,
                                &name,
                                &desc,
                                &load_state,
                                &active_state,
                                &sub_state,
                                &follower,
                                &object_path,
                                &job_id,
                                &job_type,
                                &job_object_path);
                if (r < 0) {
                        return r;
                }

                AgentUnitInfo *info = agent_ensure_unit_info(agent, name);
                if (info) {
                        assert(streq(info->object_path, object_path));
                        info->loaded = true;
                        info->active_state = active_state_from_string(active_state);
                        info->substate = strdup(sub_state);
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        return r;
                }
        }

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                return r;
        }

        return 0;
}

static bool ensure_orch_address(Agent *agent) {
        int r = 0;

        if (agent->orch_addr != NULL) {
                return true;
        }
        if (agent->host == NULL) {
                bc_log_errorf("No manager host specified for agent '%s'", agent->name);
                return false;
        }

        char *ip_address = agent->host;
        bool host_is_ipv4 = is_ipv4(ip_address);
        bool host_is_ipv6 = is_ipv6(ip_address);

        _cleanup_free_ char *resolved_ip_address = NULL;
        if (!host_is_ipv4 && !host_is_ipv6) {
                int r = get_address(ip_address, &resolved_ip_address, getaddrinfo);
                if (r < 0) {
                        bc_log_errorf("Failed to get IP address from host '%s': %s", agent->host, strerror(-r));
                        return false;
                }
                bc_log_infof("Translated '%s' to '%s'", ip_address, resolved_ip_address);
                ip_address = resolved_ip_address;
        }

        if (host_is_ipv4 || is_ipv4(ip_address)) {
                struct sockaddr_in host;
                memset(&host, 0, sizeof(host));
                host.sin_family = AF_INET;
                host.sin_port = htons(agent->port);
                r = inet_pton(AF_INET, ip_address, &host.sin_addr);
                if (r < 1) {
                        bc_log_errorf("INET4: Invalid host option '%s'", ip_address);
                        return false;
                }
                _cleanup_free_ char *orch_addr = assemble_tcp_address(&host);
                if (orch_addr == NULL) {
                        return false;
                }
                agent_set_orch_address(agent, orch_addr);
        } else if (host_is_ipv6 || is_ipv6(ip_address)) {
                struct sockaddr_in6 host6;
                memset(&host6, 0, sizeof(host6));
                host6.sin6_family = AF_INET6;
                host6.sin6_port = htons(agent->port);
                r = inet_pton(AF_INET6, ip_address, &host6.sin6_addr);
                if (r < 1) {
                        bc_log_errorf("INET6: Invalid host option '%s'", ip_address);
                        return false;
                }
                _cleanup_free_ char *orch_addr = assemble_tcp_address_v6(&host6);
                if (orch_addr == NULL) {
                        return false;
                }
                agent_set_orch_address(agent, orch_addr);
        } else {
                bc_log_errorf("Unknown protocol for '%s'", ip_address);
        }

        return agent->orch_addr != NULL;
}

bool agent_start(Agent *agent) {
        int r = 0;
        sd_bus_error error = SD_BUS_ERROR_NULL;

        bc_log_infof("Starting bluechi-agent %s", CONFIG_H_BC_VERSION);

        if (agent == NULL) {
                return false;
        }

        if (agent->name == NULL) {
                bc_log_error("No agent name specified");
                return false;
        }

        if (!ensure_orch_address(agent)) {
                return false;
        }

        /* If systemd --user, we need to be on the user bus for the proxy to work */
        if (agent->systemd_user || ALWAYS_USER_API_BUS) {
                agent->api_bus = user_bus_open(agent->event);
        } else {
                agent->api_bus = system_bus_open(agent->event);
        }

        if (agent->api_bus == NULL) {
                bc_log_error("Failed to open api dbus");
                return false;
        }

        r = sd_bus_add_object_vtable(
                        agent->api_bus, NULL, BC_AGENT_OBJECT_PATH, AGENT_INTERFACE, agent_vtable, agent);
        if (r < 0) {
                bc_log_errorf("Failed to add agent vtable: %s", strerror(-r));
                return false;
        }

        r = sd_bus_request_name(agent->api_bus, agent->api_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                bc_log_errorf("Failed to acquire service name on api dbus: %s", strerror(-r));
                return false;
        }

        if (agent->systemd_user) {
                agent->systemd_dbus = user_bus_open(agent->event);
        } else {
                agent->systemd_dbus = systemd_bus_open(agent->event);
        }
        if (agent->systemd_dbus == NULL) {
                bc_log_error("Failed to open systemd dbus");
                return false;
        }

        _cleanup_sd_bus_message_ sd_bus_message *sub_m = NULL;
        r = sd_bus_call_method(
                        agent->systemd_dbus,
                        SYSTEMD_BUS_NAME,
                        SYSTEMD_OBJECT_PATH,
                        SYSTEMD_MANAGER_IFACE,
                        "Subscribe",
                        &error,
                        &sub_m,
                        "");
        if (r < 0) {
                bc_log_errorf("Failed to issue subscribe call: %s", error.message);
                sd_bus_error_free(&error);
                return false;
        }

        r = sd_bus_add_match(
                        agent->systemd_dbus,
                        NULL,
                        "type='signal',sender='org.freedesktop.systemd1',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path_namespace='/org/freedesktop/systemd1/job'",
                        agent_match_job_changed,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to add match: %s", strerror(-r));
                return false;
        }

        r = sd_bus_add_match(
                        agent->systemd_dbus,
                        NULL,
                        "type='signal',sender='org.freedesktop.systemd1',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path_namespace='/org/freedesktop/systemd1/unit'",
                        agent_match_unit_changed,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to add match: %s", strerror(-r));
                return false;
        }

        r = sd_bus_match_signal(
                        agent->systemd_dbus,
                        NULL,
                        SYSTEMD_BUS_NAME,
                        SYSTEMD_OBJECT_PATH,
                        SYSTEMD_MANAGER_IFACE,
                        "UnitNew",
                        agent_match_unit_new,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to add unit-new peer bus match: %s", strerror(-r));
                return false;
        }

        r = sd_bus_match_signal(
                        agent->systemd_dbus,
                        NULL,
                        SYSTEMD_BUS_NAME,
                        SYSTEMD_OBJECT_PATH,
                        SYSTEMD_MANAGER_IFACE,
                        "UnitRemoved",
                        agent_match_unit_removed,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to add unit-removed peer bus match: %s", strerror(-r));
                return false;
        }

        r = sd_bus_match_signal(
                        agent->systemd_dbus,
                        NULL,
                        SYSTEMD_BUS_NAME,
                        SYSTEMD_OBJECT_PATH,
                        SYSTEMD_MANAGER_IFACE,
                        "JobRemoved",
                        agent_match_job_removed,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to add job-removed peer bus match: %s", strerror(-r));
                return false;
        }

        _cleanup_sd_bus_message_ sd_bus_message *list_units_m = NULL;
        r = sd_bus_call_method(
                        agent->systemd_dbus,
                        SYSTEMD_BUS_NAME,
                        SYSTEMD_OBJECT_PATH,
                        SYSTEMD_MANAGER_IFACE,
                        "ListUnits",
                        &error,
                        &list_units_m,
                        "");
        if (r < 0) {
                bc_log_errorf("Failed to issue list_units call: %s", error.message);
                sd_bus_error_free(&error);
                return false;
        }

        r = agent_init_units(agent, list_units_m);
        if (r < 0) {
                return false;
        }

        if (DEBUG_SYSTEMD_MESSAGES) {
                sd_bus_add_filter(agent->systemd_dbus, NULL, debug_systemd_message_handler, agent);
        }

        r = shutdown_service_register(agent->api_bus, agent->event);
        if (r < 0) {
                bc_log_errorf("Failed to register shutdown service: %s", strerror(-r));
                return false;
        }

        r = event_loop_add_shutdown_signals(agent->event);
        if (r < 0) {
                bc_log_errorf("Failed to add signals to agent event loop: %s", strerror(-r));
                return false;
        }

        r = agent_setup_heartbeat_timer(agent);
        if (r < 0) {
                bc_log_errorf("Failed to set up agent heartbeat timer: %s", strerror(-r));
                return false;
        }

        if (!agent_connect(agent)) {
                bc_log_error("Initial manager connection failed, retrying");
                agent->connection_state = AGENT_CONNECTION_STATE_RETRY;
        }

        r = sd_event_loop(agent->event);
        if (r < 0) {
                bc_log_errorf("Starting event loop failed: %s", strerror(-r));
                return false;
        }

        return true;
}

bool agent_stop(Agent *agent) {
        if (agent == NULL) {
                return false;
        }

        ProxyService *proxy = NULL;
        ProxyService *next_proxy = NULL;
        LIST_FOREACH_SAFE(proxy_services, proxy, next_proxy, agent->proxy_services) {
                agent_remove_proxy(agent, proxy, false);
        }

        return true;
}

static bool agent_connect(Agent *agent) {
        peer_bus_close(agent->peer_dbus);

        bc_log_infof("Connecting to manager on %s", agent->orch_addr);

        agent->peer_dbus = peer_bus_open(agent->event, "peer-bus-to-controller", agent->orch_addr);
        if (agent->peer_dbus == NULL) {
                bc_log_error("Failed to open peer dbus");
                return false;
        }

        int r = bus_socket_set_no_delay(agent->peer_dbus);
        if (r < 0) {
                bc_log_warn("Failed to set NO_DELAY on socket");
        }

        r = bus_socket_set_keepalive(agent->peer_dbus);
        if (r < 0) {
                bc_log_warn("Failed to set KEEPALIVE on socket");
        }

        r = sd_bus_add_object_vtable(
                        agent->peer_dbus,
                        NULL,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        internal_agent_vtable,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to add agent vtable: %s", strerror(-r));
                return false;
        }

        _cleanup_sd_bus_message_ sd_bus_message *bus_msg = NULL;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        r = sd_bus_call_method(
                        agent->peer_dbus,
                        BC_DBUS_NAME,
                        INTERNAL_MANAGER_OBJECT_PATH,
                        INTERNAL_MANAGER_INTERFACE,
                        "Register",
                        &error,
                        &bus_msg,
                        "s",
                        agent->name);
        if (r < 0) {
                bc_log_errorf("Registering as '%s' failed: %s", agent->name, error.message);
                sd_bus_error_free(&error);
                return false;
        }

        r = sd_bus_message_read(bus_msg, "");
        if (r < 0) {
                bc_log_errorf("Failed to parse response message: %s", strerror(-r));
                return false;
        }

        bc_log_infof("Connected to manager as '%s'", agent->name);

        agent->connection_state = AGENT_CONNECTION_STATE_CONNECTED;
        agent->connection_retry_count = 0;
        agent->disconnect_timestamp = 0;

        r = sd_bus_emit_properties_changed(
                        agent->api_bus, BC_AGENT_OBJECT_PATH, AGENT_INTERFACE, "Status", NULL);
        if (r < 0) {
                bc_log_errorf("Failed to emit status property changed: %s", strerror(-r));
        }

        r = sd_bus_match_signal_async(
                        agent->peer_dbus,
                        NULL,
                        "org.freedesktop.DBus.Local",
                        "/org/freedesktop/DBus/Local",
                        "org.freedesktop.DBus.Local",
                        "Disconnected",
                        agent_disconnected,
                        NULL,
                        agent);
        if (r < 0) {
                bc_log_errorf("Failed to request match for Disconnected signal: %s", strerror(-r));
                return false;
        }

        /* re-emit ProxyNew signals */

        ProxyService *proxy = NULL;
        ProxyService *next_proxy = NULL;
        LIST_FOREACH_SAFE(proxy_services, proxy, next_proxy, agent->proxy_services) {
                r = proxy_service_emit_proxy_new(proxy);
                if (r < 0) {
                        bc_log_errorf("Failed to re-emit ProxyNew signal for proxy %u requesting unit %s on %s",
                                      proxy->id,
                                      proxy->unit_name,
                                      proxy->node_name);
                }
        }

        return true;
}

static bool agent_reconnect(Agent *agent) {
        // resolve FQDN again in case the system changed
        // e.g. bluechi controller has been migrated to a different host
        if (agent->orch_addr != NULL) {
                free(agent->orch_addr);
                agent->orch_addr = NULL;
        }
        if (!ensure_orch_address(agent)) {
                return false;
        }

        return agent_connect(agent);
}

static int stop_proxy_callback(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;

        if (sd_bus_message_is_method_error(m, NULL)) {
                bc_log_errorf("Error stopping proxy service: %s", sd_bus_message_get_error(m)->message);
        }

        return 0;
}

/* Force stop the service via systemd */
static int agent_stop_local_proxy_service(Agent *agent, ProxyService *proxy) {
        if (proxy->dont_stop_proxy) {
                return 0;
        }

        bc_log_infof("Stopping proxy service %s", proxy->local_service_name);

        /* Don't stop twice */
        proxy->dont_stop_proxy = true;
        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, NULL, "StopUnit");
        if (req == NULL) {
                return -ENOMEM;
        }

        int r = sd_bus_message_append(req->message, "ss", proxy->local_service_name, "replace");
        if (r < 0) {
                return -ENOMEM;
        }

        if (!systemd_request_start(req, stop_proxy_callback)) {
                return -EIO;
        }


        return 0;
}

int agent_send_job_metrics(Agent *agent, char *unit, char *method, uint64_t systemd_job_time) {
        bc_log_debugf("Sending agent %s job metrics on unit %s: %ldus", unit, method, systemd_job_time);
        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_METRICS_OBJECT_PATH,
                        INTERNAL_AGENT_METRICS_INTERFACE,
                        "AgentJobMetrics",
                        "sst",
                        unit,
                        method,
                        systemd_job_time);
        if (r < 0) {
                bc_log_errorf("Failed to emit metric signal: %s", strerror(-r));
        }

        return 0;
}
