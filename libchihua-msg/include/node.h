#ifndef _BLUE_CHIHUAHUA_LIB_NODE
#define _BLUE_CHIHUAHUA_LIB_NODE

#include "./common/memory.h"

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <systemd/sd-event.h>

typedef struct {
        struct sockaddr_in *orch_addr;
} NodeParams;

typedef struct {
        char *orch_addr;
        sd_event *event_loop;
} Node;

Node *node_new(const NodeParams *params);
void node_unrefp(Node **node);

bool node_start(const Node *node);
bool node_stop(const Node *node);

#define _cleanup_node_ _cleanup_(node_unrefp)

#endif