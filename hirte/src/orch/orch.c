#include <stdio.h>
#include <stdlib.h>

#include "../../../libhirte/include/common/common.h"
#include "../../../libhirte/include/orchestrator.h"
#include "../ini/config.h"
#include "opt.h"

int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from orchestrator!\n");

        uint16_t accept_port = 0;
        char *config_path = NULL;

        get_opts(argc, argv, &config_path);

        struct hashmap *ini_hashmap = NULL;
        ini_hashmap = parsing_ini_file(config_path);
        if (ini_hashmap == NULL) {
                return EXIT_FAILURE;
        }

        struct hashmap *kv = hashmap_get(ini_hashmap, &(topic){ .topic = "Orchestrator" });
        OrchestratorParams orch_params = { .port = (char *) hashmap_get(kv, &(keyValue){ .key = "Port" }) };
        _cleanup_orchestrator_ Orchestrator *orchestrator = orch_new(&orch_params);
        if (orch_start(orchestrator)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
