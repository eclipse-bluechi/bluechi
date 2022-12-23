#include "../libchihua-msg/node.h"
#include "../common/dbus.h"
#include "dbus.h"
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from node!\n");
        {
                _cleanup_node_ Node *a = node_new(NULL);
                _cleanup_node_ Node *o = node_new(NULL);
                node_start(o);
        }


        struct sockaddr_in host;
        memset(&host, 0, sizeof(host));
        host.sin_family = AF_INET;
        host.sin_addr.s_addr = htonl(INADDR_ANY);
        host.sin_port = 0;

        get_opts(argc, argv, &host);

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        _cleanup_sd_bus_ sd_bus *orch = NULL;
        r = setup_peer_dbus(orch, &host);
        if (r < 0) {
                return EXIT_FAILURE;
        }

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Event loop failed: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }
        if (r < 0) {
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}