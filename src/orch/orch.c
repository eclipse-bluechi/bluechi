#include "../common/dbus.h"
#include "../ini/ini.h"
#include "../libchihua-msg/orchestrator.h"
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
        _cleanup_free_ char *configPath = NULL;

        get_opts(argc, argv, &accept_port, &configPath);

        OrchestratorParams p = { port : accept_port };
        _cleanup_orchestrator_ Orchestrator *a = orch_new(&p);
        if (orch_start(a)) {
                return EXIT_SUCCESS;
        }

        // TODO: move upwards and implement ini to OrchestratorParams in ini_handler_func
        int r = ini_parse(configPath, ini_handler_func, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to parse INI file: %s\n", configPath);
                return EXIT_FAILURE;
        }

        return EXIT_FAILURE;
}