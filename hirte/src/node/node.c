#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../libhirte/include/node.h"
#include "../../../libhirte/include/service/shutdown.h"
#include "../ini/config.h"
#include "opt.h"


int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from node!\n");

        struct sockaddr_in host;
        memset(&host, 0, sizeof(host));
        host.sin_family = AF_INET;
        host.sin_addr.s_addr = htonl(INADDR_ANY);
        host.sin_port = 0;

        get_opts(argc, argv, &host);

        char *ini_file_location = NULL;
        struct hashmap *ini_hashmap = NULL;
        ini_hashmap = parsing_ini_file(ini_file_location);
        if (ini_hashmap == NULL) {
                return EXIT_FAILURE;
        }

        _cleanup_node_ Node *node = node_new(&host, NODE_SERVICE_DEFAULT_NAME);
        if (node == NULL) {
                return EXIT_FAILURE;
        }

        if (!service_register_shutdown(node->user_dbus, node->event_loop)) {
                fprintf(stderr, "Failed to register shutdown service\n");
                return EXIT_FAILURE;
        }

        if (node_start(node)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}