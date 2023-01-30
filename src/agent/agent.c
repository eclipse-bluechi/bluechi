#include <arpa/inet.h>
#include <stdio.h>

#include "libhirte/bus/peer-bus.h"
#include "libhirte/bus/systemd-bus.h"
#include "libhirte/bus/user-bus.h"
#include "libhirte/common/common.h"
#include "libhirte/common/opt.h"
#include "libhirte/common/parse-util.h"
#include "libhirte/ini/config.h"
#include "libhirte/service/shutdown.h"

#include "agent.h"

Agent *agent_new(void) {
        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = strdup(HIRTE_AGENT_DBUS_NAME);
        if (service_name == NULL) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }

        Agent *n = malloc0(sizeof(Agent));
        n->ref_count = 1;
        n->event = steal_pointer(&event);
        n->user_bus_service_name = steal_pointer(&service_name);
        n->port = HIRTE_DEFAULT_PORT;

        return n;
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

        free(agent->name);
        free(agent->host);
        free(agent->orch_addr);
        free(agent->user_bus_service_name);

        if (agent->event != NULL) {
                sd_event_unrefp(&agent->event);
        }

        if (agent->peer_dbus != NULL) {
                sd_bus_unrefp(&agent->peer_dbus);
        }
        if (agent->user_dbus != NULL) {
                sd_bus_unrefp(&agent->user_dbus);
        }
        if (agent->systemd_dbus != NULL) {
                sd_bus_unrefp(&agent->systemd_dbus);
        }

        free(agent);
}

void agent_unrefp(Agent **agentp) {
        if (agentp && *agentp) {
                agent_unref(*agentp);
                *agentp = NULL;
        }
}

bool agent_set_port(Agent *agent, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                fprintf(stderr, "Invalid port format '%s'\n", port_s);
                return false;
        }
        agent->port = port;
        return true;
}

bool agent_set_host(Agent *agent, const char *host) {
        char *dup = strdup(host);
        if (dup == NULL) {
                fprintf(stderr, "Out of memory\n");
                return false;
        }
        free(agent->host);
        agent->host = dup;
        return true;
}

bool agent_set_name(Agent *agent, const char *name) {
        char *dup = strdup(name);
        if (dup == NULL) {
                fprintf(stderr, "Out of memory\n");
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

        topic = config_lookup_topic(config, "Agent");
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

static int agent_method_list_units(
                UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "ListUnit is not implemented yet");
}

static const sd_bus_vtable internal_agent_vtable[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("ListUnits", "", "a(ssssssouso)", agent_method_list_units, 0),
        SD_BUS_VTABLE_END
};


bool agent_start(Agent *agent) {
        struct sockaddr_in host;
        int r = 0;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *m = NULL;

        fprintf(stdout, "Starting Agent...\n");

        if (agent == NULL) {
                return false;
        }

        memset(&host, 0, sizeof(host));
        host.sin_family = AF_INET;
        host.sin_port = htons(agent->port);

        if (agent->name == NULL) {
                fprintf(stderr, "No name specified\n");
                return false;
        }

        if (agent->host == NULL) {
                fprintf(stderr, "No host specified\n");
                return false;
        }

        r = inet_pton(AF_INET, agent->host, &host.sin_addr);
        if (r < 1) {
                fprintf(stderr, "Invalid host option '%s'\n", optarg);
                return false;
        }

        agent->orch_addr = assemble_tcp_address(&host);
        if (agent->orch_addr == NULL) {
                return false;
        }

        agent->user_dbus = user_bus_open(agent->event);
        if (agent->user_dbus == NULL) {
                fprintf(stderr, "Failed to open user dbus\n");
                return false;
        }

        r = sd_bus_request_name(agent->user_dbus, agent->user_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                fprintf(stderr, "Failed to acquire service name on user dbus: %s\n", strerror(-r));
                return false;
        }

        agent->systemd_dbus = systemd_bus_open(agent->event);
        if (agent->systemd_dbus == NULL) {
                fprintf(stderr, "Failed to open systemd dbus\n");
                return false;
        }

        agent->peer_dbus = peer_bus_open(agent->event, "peer-bus-to-orchestrator", agent->orch_addr);
        if (agent->peer_dbus == NULL) {
                fprintf(stderr, "Failed to open peer dbus\n");
                return false;
        }

        r = sd_bus_add_object_vtable(
                        agent->peer_dbus,
                        NULL,
                        INTERNAL_AGENT_OBJECT_PATH,
                        INTERNAL_AGENT_INTERFACE,
                        internal_agent_vtable,
                        agent);
        if (r < 0) {
                fprintf(stderr, "Failed to add manager vtable: %s\n", strerror(-r));
                return false;
        }

        r = sd_bus_call_method(
                        agent->peer_dbus,
                        HIRTE_DBUS_NAME,
                        INTERNAL_MANAGER_OBJECT_PATH,
                        INTERNAL_MANAGER_INTERFACE,
                        "Register",
                        &error,
                        &m,
                        "s",
                        agent->name);
        if (r < 0) {
                fprintf(stderr, "Failed to issue method call: %s\n", error.message);
                sd_bus_error_free(&error);
                return false;
        }

        r = sd_bus_message_read(m, "");
        if (r < 0) {
                fprintf(stderr, "Failed to parse response message: %s\n", strerror(-r));
                return false;
        }

        printf("Registered as '%s'\n", agent->name);

        r = shutdown_service_register(agent->user_dbus, agent->event);
        if (r < 0) {
                fprintf(stderr, "Failed to register shutdown service\n");
                return false;
        }

        r = event_loop_add_shutdown_signals(agent->event);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to agent event loop\n");
                return false;
        }

        r = sd_event_loop(agent->event);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool agent_stop(Agent *agent) {
        fprintf(stdout, "Stopping Agent...\n");

        if (agent == NULL) {
                return false;
        }

        return true;
}
