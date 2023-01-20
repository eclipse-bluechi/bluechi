#include <stdio.h>
#include <stdlib.h>

#include "../../../libhirte/include/orchestrator.h"
#include "../../../libhirte/include/service/shutdown.h"
#include "../ini/config.h"
#include "opt.h"

int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from orchestrator!\n");

        uint16_t accept_port = 0;
        char *config_path = NULL;

        get_opts(argc, argv, &accept_port, &config_path);

        struct hashmap *ini_hashmap = NULL;
        ini_hashmap = parsing_ini_file(config_path);
        if (ini_hashmap == NULL) {
                return EXIT_FAILURE;
        }

        _cleanup_orchestrator_ Orchestrator *orchestrator = orch_new(
                        accept_port, ORCHESTRATOR_SERVICE_DEFAULT_NAME);
        if (orchestrator == NULL) {
                return EXIT_FAILURE;
        }

        if (!service_register_shutdown(orchestrator->user_dbus, orchestrator->event_loop)) {
                fprintf(stderr, "Failed to register shutdown service\n");
                return EXIT_FAILURE;
        }

        if (orch_start(orchestrator)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
