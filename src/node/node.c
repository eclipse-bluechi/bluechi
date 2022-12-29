#include "../libchihua-msg/node.h"
#include "../common/dbus.h"
#include "dbus.h"
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from node!\n");

        struct sockaddr_in host;
        memset(&host, 0, sizeof(host));
        host.sin_family = AF_INET;
        host.sin_addr.s_addr = htonl(INADDR_ANY);
        host.sin_port = 0;

        get_opts(argc, argv, &host);

        NodeParams p = { orch_addr : &host };
        _cleanup_node_ Node *node = node_new(&p);
        if (node_start(node)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}