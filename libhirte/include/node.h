#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "common/memory.h"

#define NODE_SERVICE_DEFAULT_NAME "org.containers.hirte.Node"
typedef struct {
        char *orch_addr;
        char *user_bus_service_name;

        sd_event *event;

        sd_bus *user_dbus;
        sd_bus *systemd_dbus;
        sd_bus *peer_dbus;
} Node;

Node *node_new(const struct sockaddr_in *peer_addr, const char *bus_service_name);
void node_unrefp(Node **node);

bool node_start(Node *node);
bool node_stop(const Node *node);

#define _cleanup_node_ _cleanup_(node_unrefp)
