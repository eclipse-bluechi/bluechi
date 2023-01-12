#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "common/memory.h"
typedef struct {
        struct sockaddr_in *orch_addr;
} NodeParams;

typedef struct {
        char *orch_addr;

        sd_event *event_loop;

        sd_bus *user_dbus;
        sd_bus *systemd_dbus;
        sd_bus *peer_dbus;
} Node;

Node *node_new(const NodeParams *params);
void node_unrefp(Node **node);

bool node_start(const Node *node);
bool node_stop(const Node *node);

#define _cleanup_node_ _cleanup_(node_unrefp)
