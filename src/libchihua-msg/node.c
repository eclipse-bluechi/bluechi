#include "node.h"
#include "../common/memory.h"

#include <stdio.h>

Node *node_new(NodeParams *p) {
        fprintf(stdout, "Creating Node...\n");
        Node *n = malloc0(sizeof(Node));
        return n;
}

void node_unrefp(Node **n) {
        fprintf(stdout, "Freeing allocated memory of Node...\n");
        free(*n);
}

void node_start(Node *o) {
        fprintf(stdout, "Starting Node...\n");
}

void node_stop(Node *o) {
        fprintf(stdout, "Stopping Node...\n");
}
