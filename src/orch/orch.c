#include "../common/dbus.h"
#include "../ini/ini.h"
#include "controller.h"
#include "opt.h"

#include <stdio.h>
#include <stdlib.h>

/* A function that takes care of printing the lines of the ini file  */
static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {
        printf("[%s] %s = %s\n", section, name, value);
        return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from orchestrator!\n");

        int port;
        _cleanup_free_ char *configPath = NULL;

        get_opts(argc, argv, &port, &configPath);

        _cleanup_sd_event_ sd_event *event = NULL;
        int r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        _cleanup_fd_ int accept_fd = -1;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;
        r = controller_setup(accept_fd, port, event, event_source);
        if (r < 0) {
                fprintf(stderr, "Failed to setup controller\n");
                return EXIT_FAILURE;
        }

        r = ini_parse(configPath, ini_handler_func, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to parse INI file: %s\n", configPath);
                return EXIT_FAILURE;
        }

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Event loop failed: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}