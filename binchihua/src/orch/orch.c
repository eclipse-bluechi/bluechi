#include "../../../libchihua-msg/include/orchestrator.h"
#include "../ini/config.h"
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>

/* A function that takes care of printing the lines of the ini file  */
// NOLINTNEXTLINE
static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {
        printf("[%s] %s = %s\n", section, name, value);
        return EXIT_FAILURE;
}

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

        OrchestratorParams orch_params = { .port = accept_port };
        _cleanup_orchestrator_ Orchestrator *orchestrator = orch_new(&orch_params);
        if (orch_start(orchestrator)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}