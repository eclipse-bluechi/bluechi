#include <arpa/inet.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../common/opt.h"
#include "../common/parse-util.h"
#include "opt.h"

const struct option options[] = { { ARG_CONFIG, required_argument, 0, ARG_CONFIG_SHORT },
                                  { NULL, 0, 0, '\0' } };

const char *get_optstring() {
        return "C:";
}

void get_opts(int argc, char *argv[], char **config_path) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, get_optstring(), options, NULL)) != -1) {
                switch (opt) {
                case ARG_CONFIG_SHORT:
                        asprintf(config_path, "%s", optarg);
                        break;
                default:
                        exit(EXIT_FAILURE);
                }
        }

        if (config_path == NULL) {
                fprintf(stderr, "Missing INI file path. Usage: %s [-c /path/to/ini-file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
}