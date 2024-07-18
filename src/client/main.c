/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>

#include "libbluechi/cli/command.h"
#include "libbluechi/common/opt.h"

#include "client.h"
#include "method-help.h"
#include "method-list-unit-files.h"
#include "method-list-units.h"
#include "method-loglevel.h"
#include "method-metrics.h"
#include "method-monitor.h"
#include "method-status.h"
#include "method-unit-actions.h"

#define OPT_NONE 0u
#define OPT_HELP 1u << 0u
#define OPT_FILTER 1u << 1u
#define OPT_FORCE 1u << 2u
#define OPT_RUNTIME 1u << 3u
#define OPT_NO_RELOAD 1u << 4u
#define OPT_WATCH 1u << 5u

int method_version(UNUSED Command *command, UNUSED void *userdata) {
        printf("bluechictl version %s\n", CONFIG_H_BC_VERSION);
        return 0;
}

const Method methods[] = {
        {"help",             0, 0,       OPT_NONE,                                method_help,            usage_bluechi},
        { "list-unit-files", 0, 1,       OPT_FILTER,                              method_list_unit_files, usage_bluechi},
        { "list-units",      0, 1,       OPT_FILTER,                              method_list_units,      usage_bluechi},
        { "start",           2, 2,       OPT_NONE,                                method_start,           usage_bluechi},
        { "stop",            2, 2,       OPT_NONE,                                method_stop,            usage_bluechi},
        { "freeze",          2, 2,       OPT_NONE,                                method_freeze,          usage_bluechi},
        { "thaw",            2, 2,       OPT_NONE,                                method_thaw,            usage_bluechi},
        { "restart",         2, 2,       OPT_NONE,                                method_restart,         usage_bluechi},
        { "reload",          2, 2,       OPT_NONE,                                method_reload,          usage_bluechi},
        { "monitor",         0, 2,       OPT_NONE,                                method_monitor,         usage_bluechi},
        { "metrics",         1, 1,       OPT_NONE,                                method_metrics,         usage_bluechi},
        { "enable",          2, ARG_ANY, OPT_FORCE | OPT_RUNTIME | OPT_NO_RELOAD, method_enable,          usage_bluechi},
        { "disable",         2, ARG_ANY, OPT_NO_RELOAD,                           method_disable,         usage_bluechi},
        { "daemon-reload",   1, 1,       OPT_NONE,                                method_daemon_reload,   usage_bluechi},
        { "status",          0, ARG_ANY, OPT_WATCH,                               method_status,          usage_bluechi},
        { "set-loglevel",    1, 2,       OPT_NONE,                                method_set_loglevel,    usage_bluechi},
        { "version",         0, 0,       OPT_NONE,                                method_version,         usage_bluechi},
        { NULL,              0, 0,       0,                                       NULL,                   NULL         }
};

const OptionType option_types[] = {
        {ARG_FILTER_SHORT,     ARG_FILTER,    OPT_FILTER   },
        { ARG_FORCE_SHORT,     ARG_FORCE,     OPT_FORCE    },
        { ARG_RUNTIME_SHORT,   ARG_RUNTIME,   OPT_RUNTIME  },
        { ARG_NO_RELOAD_SHORT, ARG_NO_RELOAD, OPT_NO_RELOAD},
        { ARG_WATCH_SHORT,     ARG_WATCH,     OPT_WATCH    },
        { 0,                   NULL,          0            }
};

#define GETOPT_OPTSTRING ARG_HELP_SHORT_S ARG_FORCE_SHORT_S ARG_WATCH_SHORT_S
const struct option getopt_options[] = {
        {ARG_HELP,       no_argument,       0, ARG_HELP_SHORT     },
        { ARG_FILTER,    required_argument, 0, ARG_FILTER_SHORT   },
        { ARG_FORCE,     no_argument,       0, ARG_FORCE_SHORT    },
        { ARG_RUNTIME,   no_argument,       0, ARG_RUNTIME_SHORT  },
        { ARG_NO_RELOAD, no_argument,       0, ARG_NO_RELOAD_SHORT},
        { ARG_WATCH,     no_argument,       0, ARG_WATCH_SHORT    },
        { NULL,          0,                 0, '\0'               }
};

static void usage() {
        method_help(NULL, NULL);
}

static int parse_cli_opts(int argc, char *argv[], Command *command) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, GETOPT_OPTSTRING, getopt_options, NULL)) != -1) {
                if (opt == ARG_HELP_SHORT) {
                        command->is_help = true;
                } else if (opt == GETOPT_UNKNOWN_OPTION) {
                        // Unrecognized option, getopt_long() prints error
                        return -EINVAL;
                } else {
                        const OptionType *option_type = get_option_type(option_types, opt);
                        assert(option_type);
                        // Recognized option, add it to the list
                        command_add_option(command, opt, optarg, option_type);
                }
        }

        if (optind < argc) {
                command->op = argv[optind++];
                command->opargv = &argv[optind];
                command->opargc = argc - optind;
        } else if (!command->is_help) {
                fprintf(stderr, "No command given\n");
                return -EINVAL;
        }

        return 0;
}

int main(int argc, char *argv[]) {
        int r = 0;
        _cleanup_command_ Command *command = new_command();
        r = parse_cli_opts(argc, argv, command);
        if (r < 0) {
                usage();
                return EXIT_FAILURE;
        }
        if (command->op == NULL && command->is_help) {
                usage();
                return EXIT_SUCCESS;
        }

        command->method = methods_get_method(command->op, methods);
        if (command->method == NULL) {
                fprintf(stderr, "Method %s not found\n", command->op);
                usage();
                return EXIT_FAILURE;
        }

        _cleanup_client_ Client *client = new_client();
        r = client_open_sd_bus(client);
        if (r < 0) {
                return EXIT_FAILURE;
        }

        r = command_execute(command, client);
        if (r < 0) {
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
