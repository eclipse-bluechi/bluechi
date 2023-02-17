#include <arpa/inet.h>
#include <errno.h>

#include "libhirte/bus/bus.h"
#include "libhirte/bus/utils.h"
#include "libhirte/common/common.h"
#include "libhirte/common/event-util.h"
#include "libhirte/common/opt.h"
#include "libhirte/common/parse-util.h"
#include "libhirte/ini/config.h"
#include "libhirte/log/log.h"
#include "libhirte/service/shutdown.h"

#include "agent.h"

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

static int agent_reset_heartbeat_timer(Agent *agent, sd_event_source **event_source);

static int agent_heartbeat_timer_callback(sd_event_source *event_source, UNUSED uint64_t usec, void *userdata) {
        Agent *agent = userdata;
        int r = 0;

        assert(event_source);

        r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "Heartbeat",
                        "s",
                        agent->name);
        if (r < 0) {
                hirte_log_errorf("Failed to emit heartbeat signal: %s", strerror(-r));
                return r;
        }

        r = agent_reset_heartbeat_timer(agent, &event_source);
        if (r < 0) {
                hirte_log_errorf("Failed to reset agent heartbeat timer: %s", strerror(-r));
                return r;
        }

        return 0;
}

static int agent_reset_heartbeat_timer(Agent *agent, sd_event_source **event_source) {
        return event_reset_time_relative(
                        agent->event,
                        event_source,
                        CLOCK_BOOTTIME,
                        AGENT_HEARTBEAT_INTERVAL_USEC,
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

        r = agent_reset_heartbeat_timer(agent, &event_source);
        if (r < 0) {
                hirte_log_errorf("Failed to reset agent heartbeat timer: %s", strerror(-r));
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
                hirte_log_errorf("Failed to create new bus message: %s", strerror(-r));
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

        int r = sd_bus_call_async(
                        agent->systemd_dbus, &req->slot, req->message, callback, req, HIRTE_DEFAULT_DBUS_TIMEOUT);
        if (r < 0) {
                hirte_log_errorf("Failed to call async: %s", strerror(-r));
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
                hirte_log_errorf("Failed to create event loop: %s", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = strdup(HIRTE_AGENT_DBUS_NAME);
        if (service_name == NULL) {
                hirte_log_error("Out of memory");
                return NULL;
        }

        _cleanup_agent_ Agent *n = malloc0(sizeof(Agent));
        n->ref_count = 1;
        n->event = steal_pointer(&event);
        n->api_bus_service_name = steal_pointer(&service_name);
        n->port = HIRTE_DEFAULT_PORT;
        LIST_HEAD_INIT(n->outstanding_requests);
        LIST_HEAD_INIT(n->tracked_jobs);

        n->unit_infos = hashmap_new(
                        sizeof(AgentUnitInfo), 0, 0, 0, unit_info_hash, unit_info_compare, unit_info_clear, NULL);
        if (n->unit_infos == NULL) {
                return NULL;
        }

        return steal_pointer(&n);
}

Agent *agent_ref(Agent *agent) {
        agent->ref_count++;
        return agent;
}

void agent_unref(Agent *agent) {
        agent->ref_count--;
        if (agent->ref_count != 0) {
                return;
        }

        hashmap_free(agent->unit_infos);

        free(agent->name);
        free(agent->host);
        free(agent->orch_addr);
        free(agent->api_bus_service_name);

        if (agent->event != NULL) {
                sd_event_unrefp(&agent->event);
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

        free(agent);
}

bool agent_set_port(Agent *agent, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                hirte_log_errorf("Invalid port format '%s'", port_s);
                return false;
        }
        agent->port = port;
        return true;
}

bool agent_set_host(Agent *agent, const char *host) {
        char *dup = strdup(host);
        if (dup == NULL) {
                hirte_log_error("Out of memory");
                return false;
        }
        free(agent->host);
        agent->host = dup;
        return true;
}

bool agent_set_name(Agent *agent, const char *name) {
        char *dup = strdup(name);
        if (dup == NULL) {
                hirte_log_error("Out of memory");
                return false;
        }

        free(agent->name);
        agent->name = dup;
        return true;
}

bool agent_parse_config(Agent *agent, const char *configfile) {
        _cleanup_config_ config *config = NULL;
        topic *topic = NULL;
        const char *name = NULL, *host = NULL, *port = NULL;

        config = parsing_ini_file(configfile);
        if (config == NULL) {
                return false;
        }

        // set defaults for logging
        hirte_log_init();
        // overwrite default log settings with config
        hirte_log_init_from_config(config);
        // overwrite config settings with env vars
        hirte_log_init_from_env();

        topic = config_lookup_topic(config, "Node");
        if (topic == NULL) {
                return true;
        }

        name = topic_lookup(topic, "Name");
        if (name) {
                if (!agent_set_name(agent, name)) {
                        return false;
                }
        }

        host = topic_lookup(topic, "Host");
        if (host) {
                if (!agent_set_host(agent, host)) {
                        return false;
                }
        }

        port = topic_lookup(topic, "Port");
        if (port) {
                if (!agent_set_port(agent, port)) {
                        return false;
                }
        }

        return true;
}

static void agent_update_unit_infos_for(Agent *agent, AgentUnitInfo *info) {
        if (!info->subscribed && !info->loaded) {
                AgentUnitInfoKey key = { info->object_path };
                hashmap_delete(agent->unit_infos, &key);
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

/*************************************************************************
 ********** org.containers.hirte.internal.Agent.ListUnits ****************
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

/*************************************************************************
 ******** org.containers.hirte.internal.Agent.GetUnitProperties **********
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


static int get_unit_properties_got_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_systemd_request_ SystemdRequest *req = userdata;
        Agent *agent = req->agent;

        if (sd_bus_message_is_method_error(m, NULL)) {
                /* Forward error */
                return sd_bus_reply_method_error(req->request_message, sd_bus_message_get_error(m));
        }

        const char *unit_path = NULL;
        int r = sd_bus_message_read(m, "o", &unit_path);
        if (r < 0) {
                return sd_bus_reply_method_errorf(req->request_message, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req2 = agent_create_request_full(
                        agent, req->request_message, unit_path, "org.freedesktop.DBus.Properties", "GetAll");
        if (req2 == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req2->message, "s", SYSTEMD_UNIT_IFACE);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req2, get_unit_properties_got_properties)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}


static int agent_method_get_unit_properties(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;

        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal Error");
        }

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, m, "GetUnit");
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        r = sd_bus_message_append(req->message, "s", unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        if (!systemd_request_start(req, get_unit_properties_got_unit)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/* Keep track of outstanding systemd job and connect it back to
   the originating hirte job id so we can proxy changes to it. */
typedef struct {
        int ref_count;

        Agent *agent;
        uint32_t hirte_job_id;
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
        free(op);
}

DEFINE_CLEANUP_FUNC(AgentJobOp, agent_job_op_unref)
#define _cleanup_agent_job_op_ _cleanup_(agent_job_op_unrefp)

static AgentJobOp *agent_job_new(Agent *agent, uint32_t hirte_job_id) {
        AgentJobOp *op = malloc0(sizeof(AgentJobOp));
        if (op) {
                op->ref_count = 1;
                op->agent = agent_ref(agent);
                op->hirte_job_id = hirte_job_id;
        }
        return op;
}


static void agent_job_done(UNUSED sd_bus_message *m, const char *result, void *userdata) {
        AgentJobOp *op = userdata;
        Agent *agent = op->agent;

        hirte_log_debugf("Sending notification JobDone with results: %s", result);

        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "JobDone",
                        "us",
                        op->hirte_job_id,
                        result);
        if (r < 0) {
                hirte_log_errorf("Failed to emit JobDone: %s", strerror(-r));
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

        hirte_log_debugf("Request to %s unit: %s - Action: %s", method, name, mode);

        _cleanup_systemd_request_ SystemdRequest *req = agent_create_request(agent, m, method);
        if (req == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        _cleanup_agent_job_op_ AgentJobOp *op = agent_job_new(agent, job_id);
        if (op == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        systemd_request_set_userdata(req, agent_job_op_ref(op), (free_func_t) agent_job_op_unref);

        r = sd_bus_message_append(req->message, "ss", name, mode);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        hirte_log_debugf("Return value: %i", r);

        if (!systemd_request_start(req, unit_lifecycle_method_callback)) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        return 1;
}

/*************************************************************************
 ********** org.containers.hirte.internal.Agent.StartUnit ****************
 ************************************************************************/

static int agent_method_start_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "StartUnit");
}

/*************************************************************************
 ********** org.containers.hirte.internal.Agent.StopUnit ****************
 ************************************************************************/

static int agent_method_stop_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "StopUnit");
}

/*************************************************************************
 ********** org.containers.hirte.internal.Agent.RestartUnit **************
 ************************************************************************/

static int agent_method_restart_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "RestartUnit");
}

/*************************************************************************
 ********** org.containers.hirte.internal.Agent.ReloadUnit ***************
 ************************************************************************/

static int agent_method_reload_unit(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        return agent_run_unit_lifecycle_method(m, (Agent *) userdata, "ReloadUnit");
}

/*************************************************************************
 ********** org.containers.hirte.internal.Agent.Subscribe ****************
 ************************************************************************/

static void agent_emit_unit_new(Agent *agent, AgentUnitInfo *info, const char *reason) {
        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "UnitNew",
                        "ss",
                        info->unit,
                        reason);
        if (r < 0) {
                hirte_log_warn("Failed to emit UnitNew");
        }
}

static void agent_emit_unit_removed(Agent *agent, AgentUnitInfo *info) {
        int r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "UnitRemoved",
                        "s",
                        info->unit);
        if (r < 0) {
                hirte_log_warn("Failed to emit UnitRemoved");
        }
}

static void agent_emit_unit_state_changed(Agent *agent, AgentUnitInfo *info, const char *reason) {
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
                hirte_log_warn("Failed to emit UnitStateChanged");
        }
}

static int agent_method_subscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
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
 ********** org.containers.hirte.internal.Agent.Unsubscribe **************
 ************************************************************************/

static int agent_method_unsubscribe(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        Agent *agent = userdata;
        const char *unit = NULL;
        int r = sd_bus_message_read(m, "s", &unit);
        if (r < 0) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_INVALID_ARGS, "Invalid arguments");
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

static const sd_bus_vtable internal_agent_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", UNIT_INFO_STRUCT_ARRAY_TYPESTRING, agent_method_list_units, 0),
        SD_BUS_METHOD("GetUnitProperties", "s", "a{sv}", agent_method_get_unit_properties, 0),
        SD_BUS_METHOD("StartUnit", "ssu", "", agent_method_start_unit, 0),
        SD_BUS_METHOD("StopUnit", "ssu", "", agent_method_stop_unit, 0),
        SD_BUS_METHOD("RestartUnit", "ssu", "", agent_method_restart_unit, 0),
        SD_BUS_METHOD("ReloadUnit", "ssu", "", agent_method_reload_unit, 0),
        SD_BUS_METHOD("Subscribe", "s", "", agent_method_subscribe, 0),
        SD_BUS_METHOD("Unsubscribe", "s", "", agent_method_unsubscribe, 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobDone", "us", SD_BUS_PARAM(id) SD_BUS_PARAM(result), 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobStateChanged", "us", SD_BUS_PARAM(id) SD_BUS_PARAM(state), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitPropertiesChanged", "sa{sv}", SD_BUS_PARAM(unit) SD_BUS_PARAM(properties), 0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitNew", "ss", SD_BUS_PARAM(unit) SD_BUS_PARAM(reason), 0),
        SD_BUS_SIGNAL_WITH_NAMES(
                        "UnitStateChanged",
                        "ssss",
                        SD_BUS_PARAM(unit) SD_BUS_PARAM(active_state) SD_BUS_PARAM(substate)
                                        SD_BUS_PARAM(reason),
                        0),
        SD_BUS_SIGNAL_WITH_NAMES("UnitRemoved", "s", SD_BUS_PARAM(unit), 0),
        SD_BUS_SIGNAL_WITH_NAMES("Heartbeat", "s", SD_BUS_PARAM(agent_name), 0),
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
                hirte_log_errorf("Failed to read job property: %s", strerror(-r));
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
                hirte_log_errorf("Failed to get job property: %s", strerror(-r));
                return r;
        }

        r = sd_bus_emit_signal(
                        agent->peer_dbus,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        "JobStateChanged",
                        "us",
                        op->hirte_job_id,
                        state);
        if (r < 0) {
                hirte_log_errorf("Failed to emit JobStateChanged: %s", strerror(-r));
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

        /* Only handle Unit iface changes */
        if (!streq(interface, "org.freedesktop.systemd1.Unit")) {
                return 0;
        }

        AgentUnitInfo *info = agent_get_unit_info(agent, sd_bus_message_get_path(m));
        if (info == NULL) {
                return 0;
        }

        bool state_changed = unit_info_update_state(info, m);
        if (state_changed && info->subscribed) {
                agent_emit_unit_state_changed(agent, info, "real");
        }

        if (!info->subscribed) {
                return 0;
        }

        /* Rewind and skip to copy content again */
        (void) sd_bus_message_rewind(m, true);
        (void) sd_bus_message_skip(m, "s"); // re-skip interface

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
        info->active_state = _UNIT_ACTIVE_STATE_INVALID;
        info->substate = NULL;

        if (info->subscribed) {
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
        info->substate = NULL;

        if (info->subscribed) {
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
                hirte_log_errorf("Can't parse job result: %s", strerror(-r));
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
        hirte_log_infof("Incoming message from systemd: path: %s, iface: %s, member: %s, signature: '%s'",
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

bool agent_start(Agent *agent) {
        struct sockaddr_in host;
        int r = 0;
        sd_bus_error error = SD_BUS_ERROR_NULL;

        if (agent == NULL) {
                return false;
        }

        memset(&host, 0, sizeof(host));
        host.sin_family = AF_INET;
        host.sin_port = htons(agent->port);

        if (agent->name == NULL) {
                hirte_log_error("No agent name specified");
                return false;
        }

        if (agent->host == NULL) {
                hirte_log_errorf("No manager host specified for agent '%s'", agent->name);
                return false;
        }

        r = inet_pton(AF_INET, agent->host, &host.sin_addr);
        if (r < 1) {
                hirte_log_errorf("Invalid host option '%s'", agent->host);
                return false;
        }

        agent->orch_addr = assemble_tcp_address(&host);
        if (agent->orch_addr == NULL) {
                return false;
        }

#ifdef USE_USER_API_BUS
        agent->api_bus = user_bus_open(agent->event);
#else
        agent->api_bus = system_bus_open(agent->event);
#endif
        if (agent->api_bus == NULL) {
                hirte_log_error("Failed to open user dbus");
                return false;
        }

        r = sd_bus_request_name(agent->api_bus, agent->api_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                hirte_log_errorf("Failed to acquire service name on user dbus: %s", strerror(-r));
                return false;
        }

        agent->systemd_dbus = systemd_bus_open(agent->event);
        if (agent->systemd_dbus == NULL) {
                hirte_log_error("Failed to open systemd dbus");
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
                hirte_log_errorf("Failed to issue subscribe call: %s", error.message);
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
                hirte_log_errorf("Failed to add match: %s", strerror(-r));
                return false;
        }

        r = sd_bus_add_match(
                        agent->systemd_dbus,
                        NULL,
                        "type='signal',sender='org.freedesktop.systemd1',interface='org.freedesktop.DBus.Properties',member='PropertiesChanged',path_namespace='/org/freedesktop/systemd1/unit'",
                        agent_match_unit_changed,
                        agent);
        if (r < 0) {
                hirte_log_error("Failed to add match");
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
                hirte_log_errorf("Failed to add unit-new peer bus match: %s", strerror(-r));
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
                hirte_log_errorf("Failed to add unit-removed peer bus match: %s", strerror(-r));
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
                hirte_log_errorf("Failed to add job-removed peer bus match: %s", strerror(-r));
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
                hirte_log_errorf("Failed to issue list_units call: %s", error.message);
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

        agent->peer_dbus = peer_bus_open(agent->event, "peer-bus-to-orchestrator", agent->orch_addr);
        if (agent->peer_dbus == NULL) {
                hirte_log_error("Failed to open peer dbus");
                return false;
        }

        r = bus_socket_set_no_delay(agent->peer_dbus);
        if (r < 0) {
                hirte_log_warn("Failed to set NO_DELAY on socket");
        }

        r = bus_socket_set_keepalive(agent->peer_dbus);
        if (r < 0) {
                hirte_log_warn("Failed to set KEEPALIVE on socket");
        }

        r = sd_bus_add_object_vtable(
                        agent->peer_dbus,
                        NULL,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        internal_agent_vtable,
                        agent);
        if (r < 0) {
                hirte_log_errorf("Failed to add manager vtable: %s", strerror(-r));
                return false;
        }

        _cleanup_sd_bus_message_ sd_bus_message *reg_m = NULL;
        r = sd_bus_call_method(
                        agent->peer_dbus,
                        HIRTE_DBUS_NAME,
                        INTERNAL_MANAGER_OBJECT_PATH,
                        INTERNAL_MANAGER_INTERFACE,
                        "Register",
                        &error,
                        &reg_m,
                        "s",
                        agent->name);
        if (r < 0) {
                hirte_log_errorf("Failed to issue method call: %s", error.message);
                sd_bus_error_free(&error);
                return false;
        }

        r = sd_bus_message_read(reg_m, "");
        if (r < 0) {
                hirte_log_errorf("Failed to parse response message: %s", strerror(-r));
                return false;
        }

        hirte_log_infof("Registered to '%s' as '%s'", agent->orch_addr, agent->name);

        r = shutdown_service_register(agent->api_bus, agent->event);
        if (r < 0) {
                hirte_log_errorf("Failed to register shutdown service: %s", strerror(-r));
                return false;
        }

        r = event_loop_add_shutdown_signals(agent->event);
        if (r < 0) {
                hirte_log_errorf("Failed to add signals to agent event loop: %s", strerror(-r));
                return false;
        }

        r = agent_setup_heartbeat_timer(agent);
        if (r < 0) {
                hirte_log_errorf("Failed to set up agent heartbeat timer: %s", strerror(-r));
                return false;
        }

        r = sd_event_loop(agent->event);
        if (r < 0) {
                hirte_log_errorf("Starting event loop failed: %s", strerror(-r));
                return false;
        }

        return true;
}

bool agent_stop(Agent *agent) {
        if (agent == NULL) {
                return false;
        }

        return true;
}
