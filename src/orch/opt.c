#include "opt.h"
#include "../common/parse-util.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
void get_opts(int argc, char *argv[], int *ctrlPort, char **configPath) {
        int opt;
        int pflag = 0;

        while ((opt = getopt(argc, argv, "p:c:")) != -1) {
                switch (opt) {
                case 'p':
                        if (parse_long(optarg, (long *) ctrlPort) != 0) {
                                fprintf(stderr, "Invalid port option '%s'\n", optarg);
                                exit(EXIT_FAILURE);
                        }
                        pflag = 1;
                        break;
                case 'c':
                        asprintf(configPath, "%s", optarg);
                        break;
                default:
                        exit(EXIT_FAILURE);
                }
        }

        if (pflag != 1) {
                fprintf(stderr, "Missing port option. Usage: %s [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        if (configPath == NULL) {
                fprintf(stderr, "Missing INI file path. Usage: %s [-c /path/to/ini-file]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
}