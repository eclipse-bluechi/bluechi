#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "libhirte/common/opt.h"
#include "libhirte/common/parse-util.h"
#include "opt.h"

const struct option options[] = { { ARG_PORT, required_argument, 0, ARG_PORT_SHORT },
                                  { ARG_CONFIG, required_argument, 0, ARG_CONFIG_SHORT },
                                  { NULL, 0, 0, '\0' } };

const char *get_optstring() {
        return "P:c:";
}

void get_opts(int argc, char *argv[], uint16_t *ctrl_port, char **config_path) {
        int opt = 0;
        int pflag = 0;

        while ((opt = getopt_long(argc, argv, get_optstring(), options, NULL)) != -1) {
                switch (opt) {
                case ARG_PORT_SHORT:
                        if (!parse_port(optarg, ctrl_port)) {
                                fprintf(stderr, "Invalid port option '%s'\n", optarg);
                                exit(EXIT_FAILURE);
                        }
                        pflag = 1;
                        break;
                case ARG_CONFIG_SHORT:
                        asprintf(config_path, "%s", optarg);
                        break;
                default:
                        exit(EXIT_FAILURE);
                }
        }

        if (pflag != 1) {
                fprintf(stderr, "Missing port option. Usage: %s [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        if (config_path == NULL) {
                fprintf(stderr, "Missing INI file path. Usage: %s [-c /path/to/ini-file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
}
