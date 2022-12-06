#include "../../../libchihua-msg/include/node.h"
#include "../ini/config.h"
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

        char *ini_file_location = "../../../doc/example.ini";
        struct hashmap *ini_hashmap = NULL;
        ini_hashmap = parsing_ini_file(ini_file_location);
        if (ini_hashmap == NULL) {
                return EXIT_FAILURE;
        }

        NodeParams node_params = { .orch_addr = &host };
        _cleanup_node_ Node *node = node_new(&node_params);
        if (node_start(node)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}