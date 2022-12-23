#ifndef _BLUE_CHIHUAHUA_LIB_NODE
#define _BLUE_CHIHUAHUA_LIB_NODE

#include "../common/dbus.h"
#include "../common/memory.h"

#include <stdbool.h>
typedef struct {

} NodeParams;

typedef struct {
        sd_event *event_loop;
} Node;

Node *node_new(NodeParams *p);
void node_unrefp(Node **n);

bool node_start(Node *n);
bool node_stop(Node *n);

#define _cleanup_node_ _cleanup_(node_unrefp)

#endif