/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/opt.h"

#include "client.h"

const struct option options[] = { { ARG_HELP, no_argument, 0, ARG_HELP_SHORT },
                                  { ARG_FILTER, required_argument, 0, ARG_FILTER_SHORT },
                                  { NULL, 0, 0, '\0' } };

#define OPTIONS_STR ARG_HELP_SHORT_S ARG_FILTER_SHORT_S

static char *op;
static char **opargv;
static int opargc;
static bool no_action = false;

static void usage(char *argv[]) {
        print_client_usage(argv[0]);
}

static int get_opts(int argc, char *argv[], char **opt_filter_glob) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, OPTIONS_STR, options, NULL)) != -1) {
                if (opt == ARG_HELP_SHORT) {
                        usage(argv);
                        no_action = true;
                        return 0;
                } else if (opt == ARG_FILTER_SHORT) {
                        *opt_filter_glob = strdup(optarg);
                } else {
                        fprintf(stderr, "Unsupported option %c\n", opt);
                        usage(argv);
                        return -EINVAL;
                }
        }

        if (optind < argc) {
                op = argv[optind++];
                opargv = &argv[optind];
                opargc = argc - optind;
        } else {
                fprintf(stderr, "No command given\n");
                usage(argv);
                return -EINVAL;
        }

        return 0;
}

int main(int argc, char *argv[]) {
        int r = 0;

        _cleanup_free_ char *opt_filter_glob = NULL;
        r = get_opts(argc, argv, &opt_filter_glob);
        if (r < 0) {
                return EXIT_FAILURE;
        }
        if (no_action) {
                return EXIT_SUCCESS;
        }

        _cleanup_client_ Client *client = new_client(op, opargc, opargv, opt_filter_glob);
        r = client_call_manager(client);
        if (r < 0) {
                fprintf(stderr, "Call to manager failed: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
