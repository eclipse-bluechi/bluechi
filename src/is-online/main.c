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

#include "help.h"
#include "is-online.h"
#include "opt.h"

int method_version(UNUSED Command *command, UNUSED void *userdata) {
        printf("bluechi-is-online version %s\n", CONFIG_H_BC_VERSION);
        return 0;
}

const Method methods[] = {
        { "help",    0, 0, OPT_NONE,                                         method_help,             usage },
        { "version", 0, 0, OPT_NONE,                                         method_version,          usage },
        { "agent",   0, 0, OPT_MONITOR | OPT_WAIT | OPT_SWITCH_CTRL_TIMEOUT, method_is_online_agent,  usage },
        { "node",    1, 1, OPT_MONITOR | OPT_WAIT,                           method_is_online_node,   usage },
        { "system",  0, 0, OPT_MONITOR | OPT_WAIT,                           method_is_online_system, usage },
        { NULL,      0, 0, 0,                                                NULL,                    NULL  }
};

const OptionType option_types[] = {
        { ARG_MONITOR_SHORT,             ARG_MONITOR,             OPT_MONITOR             },
        { ARG_WAIT_SHORT,                ARG_WAIT,                OPT_WAIT                },
        { ARG_SWITCH_CTRL_TIMEOUT_SHORT, ARG_SWITCH_CTRL_TIMEOUT, OPT_SWITCH_CTRL_TIMEOUT },
        { 0,                             NULL,                    0                       }
};

#define GETOPT_OPTSTRING ARG_HELP_SHORT_S
const struct option getopt_options[] = {
        { ARG_HELP,                no_argument,       0, ARG_HELP_SHORT                },
        { ARG_MONITOR,             no_argument,       0, ARG_MONITOR_SHORT             },
        { ARG_WAIT,                required_argument, 0, ARG_WAIT_SHORT                },
        { ARG_SWITCH_CTRL_TIMEOUT, required_argument, 0, ARG_SWITCH_CTRL_TIMEOUT_SHORT },
        { NULL,                    0,                 0, '\0'                          }
};

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

        r = command_execute(command, NULL);
        if (r < 0) {
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
