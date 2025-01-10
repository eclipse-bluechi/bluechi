/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

#include "libbluechi/common/opt.h"
#include "libbluechi/log/log.h"

#include "controller.h"

const struct option options[] = {
        { ARG_PORT,    required_argument, 0, ARG_PORT_SHORT    },
        { ARG_CONFIG,  required_argument, 0, ARG_CONFIG_SHORT  },
        { ARG_HELP,    no_argument,       0, ARG_HELP_SHORT    },
        { ARG_VERSION, no_argument,       0, ARG_VERSION_SHORT },
        { NULL,        0,                 0, '\0'              }
};

#define GETOPT_OPTSTRING ARG_PORT_SHORT_S ARG_HELP_SHORT_S ARG_CONFIG_SHORT_S ARG_VERSION_SHORT_S

static const char *opt_port = 0;
static const char *opt_config = NULL;

static void usage(char *argv[]) {
        printf("Usage:\n"
               "\t%s [options...] \n"
               "Available options are:\n"
               "\t-%c %s\t\t Print this help message.\n"
               "\t-%c %s\t\t The port of bluechi to connect to.\n"
               "\t-%c %s\t A path to a config file used to bootstrap bluechi-controller.\n"
               "\t-%c %s\t Print current bluechi version.\n",
               argv[0],
               ARG_HELP_SHORT,
               ARG_HELP,
               ARG_PORT_SHORT,
               ARG_PORT,
               ARG_CONFIG_SHORT,
               ARG_CONFIG,
               ARG_VERSION_SHORT,
               ARG_VERSION);
}

static int get_opts(int argc, char *argv[]) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, GETOPT_OPTSTRING, options, NULL)) != -1) {
                switch (opt) {
                case ARG_HELP_SHORT:
                        usage(argv);
                        return 1;

                case ARG_VERSION_SHORT:
                        printf("bluechi-controller version %s\n", CONFIG_H_BC_VERSION);
                        return 1;

                case ARG_PORT_SHORT:
                        opt_port = optarg;
                        break;

                case ARG_CONFIG_SHORT:
                        opt_config = optarg;
                        break;

                default:
                        fprintf(stderr, "Unsupported option %c\n", opt);
                        usage(argv);
                        return -EINVAL;
                }
        }

        return 0;
}


int main(int argc, char *argv[]) {
        int r = get_opts(argc, argv);
        if (r < 0) {
                return EXIT_FAILURE;
        } else if (r > 0) {
                return EXIT_SUCCESS;
        }

        _cleanup_controller_ Controller *controller = controller_new();
        if (controller == NULL) {
                return EXIT_FAILURE;
        }

        if (!controller_parse_config(controller, opt_config)) {
                return EXIT_FAILURE;
        }

        bc_log_init(controller->config);
        _cleanup_free_ const char *dumped_cfg = cfg_dump(controller->config);
        bc_log_debugf("Final configuration used:\n%s", dumped_cfg);

        if (!controller_apply_config(controller)) {
                return EXIT_FAILURE;
        }

        /* Override individual options */

        if (opt_port && !controller_set_port(controller, opt_port)) {
                return EXIT_FAILURE;
        }

        if (controller_start(controller)) {
                return EXIT_SUCCESS;
        }
        controller_stop(controller);
        return EXIT_FAILURE;
}
