/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

#include "libhirte/common/opt.h"
#include "libhirte/common/parse-util.h"
#include "libhirte/ini/config.h"
#include "libhirte/service/shutdown.h"

#include "manager.h"

const struct option options[] = { { ARG_PORT, required_argument, 0, ARG_PORT_SHORT },
                                  { ARG_CONFIG, required_argument, 0, ARG_CONFIG_SHORT },
                                  { ARG_HELP, no_argument, 0, ARG_HELP_SHORT },
                                  { NULL, 0, 0, '\0' } };

#define OPTIONS_STR ARG_PORT_SHORT_S ARG_HELP_SHORT_S ARG_CONFIG_SHORT_S

static const char *opt_port = 0;
static const char *opt_config = NULL;

static void usage(char *argv[]) {
        printf("Usage: %s [-p port] [-c config]\n", argv[0]);
}

static int get_opts(int argc, char *argv[]) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, OPTIONS_STR, options, NULL)) != -1) {
                switch (opt) {
                case ARG_HELP_SHORT:
                        usage(argv);
                        return 0;

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
        }

        _cleanup_manager_ Manager *manager = manager_new();
        if (manager == NULL) {
                return EXIT_FAILURE;
        }

        /* First load config */
        if (opt_config) {
                if (!manager_parse_config(manager, opt_config)) {
                        return EXIT_FAILURE;
                }
        }

        /* Then override individual options */

        if (opt_port && !manager_set_port(manager, opt_port)) {
                return EXIT_FAILURE;
        }

        if (manager_start(manager)) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
