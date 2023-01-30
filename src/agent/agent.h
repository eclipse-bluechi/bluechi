#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>

#include "libhirte/common/common.h"

typedef struct {
        int ref_count;

        char *name;
        char *host;
        int port;

        char *orch_addr;
        char *user_bus_service_name;

        sd_event *event;

        sd_bus *user_dbus;
        sd_bus *systemd_dbus;
        sd_bus *peer_dbus;
} Agent;

Agent *agent_new(void);
Agent *agent_ref(Agent *agent);
void agent_unref(Agent *agent);
void agent_unrefp(Agent **agentp);

bool agent_set_port(Agent *agent, const char *port);
bool agent_set_host(Agent *agent, const char *host);
bool agent_set_name(Agent *agent, const char *name);
bool agent_parse_config(Agent *agent, const char *configfile);

bool agent_start(Agent *agent);
bool agent_stop(Agent *agent);

#define _cleanup_agent_ _cleanup_(agent_unrefp)
