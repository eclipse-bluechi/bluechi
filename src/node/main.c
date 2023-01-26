#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"

#include "libhirte/common/opt.h"

const struct option options[] = { { ARG_HOST, required_argument, 0, ARG_HOST_SHORT },
                                  { ARG_PORT, required_argument, 0, ARG_PORT_SHORT },
                                  { ARG_NAME, required_argument, 0, ARG_NAME_SHORT },
                                  { ARG_CONFIG, required_argument, 0, ARG_CONFIG_SHORT },
                                  { ARG_HELP, no_argument, 0, ARG_HELP_SHORT },
                                  { NULL, 0, 0, '\0' } };

#define OPTIONS_STR ARG_PORT_SHORT_S ARG_HOST_SHORT_S ARG_HELP_SHORT_S ARG_CONFIG_SHORT_S ARG_NAME_SHORT_S

static const char *opt_port = 0;
static const char *opt_host = NULL;
static const char *opt_name = NULL;
static const char *opt_config = NULL;

static void usage(char *argv[]) {
        fprintf(stderr, "Usage: %s [-H host] [-p port] [-c config] [-n name]\n", argv[0]);
}

static void get_opts(int argc, char *argv[]) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, OPTIONS_STR, options, NULL)) != -1) {
                switch (opt) {
                case ARG_HELP_SHORT:
                        usage(argv);
                        exit(EXIT_SUCCESS);
                        break;

                case ARG_NAME_SHORT:
                        opt_name = optarg;
                        break;

                case ARG_HOST_SHORT:
                        opt_host = optarg;
                        break;

                case ARG_CONFIG_SHORT:
                        opt_config = optarg;
                        break;

                case ARG_PORT_SHORT:
                        opt_port = optarg;
                        break;

                default:
                        fprintf(stderr, "Unsupported option %c\n", opt);
                        usage(argv);
                        exit(EXIT_FAILURE);
                }
        }
}

int main(int argc, char *argv[]) {
        fprintf(stdout, "Hello from node!\n");

        get_opts(argc, argv);

        _cleanup_node_ Node *node = node_new();
        if (node == NULL) {
                return EXIT_FAILURE;
        }

        /* First load config */
        if (opt_config) {
                if (!node_parse_config(node, opt_config)) {
                        return EXIT_FAILURE;
                }
        }

        /* Then override individual options */

        if (opt_port && !node_set_port(node, opt_port)) {
                return EXIT_FAILURE;
        }

        if (opt_host && !node_set_host(node, opt_host)) {
                return EXIT_FAILURE;
        }

        if (opt_name && !node_set_name(node, opt_name)) {
                return EXIT_FAILURE;
        }

        if (node_start(node)) {
                return EXIT_SUCCESS;
        }

        fprintf(stdout, "Node exited\n");

        return EXIT_FAILURE;
}
