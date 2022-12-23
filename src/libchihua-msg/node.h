#ifndef _BLUE_CHIHUAHUA_LIB_NODE
#define _BLUE_CHIHUAHUA_LIB_NODE

typedef struct {

} Node;

typedef struct {

} NodeParams;

Node *node_new(NodeParams *p);
void node_start(Node *n);
void node_stop(Node *n);

#endif