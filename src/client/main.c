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
#include "method-daemon-reload.h"
#include "method-default-target.h"
#include "method-enable-disable.h"
#include "method-freeze-thaw.h"
#include "method-help.h"
#include "method-is-enabled.h"
#include "method-kill.h"
#include "method-list-unit-files.h"
#include "method-list-units.h"
#include "method-loglevel.h"
#include "method-metrics.h"
#include "method-monitor.h"
#include "method-reset-failed.h"
#include "method-status.h"
#include "method-unit-lifecycle.h"

#define OPT_NONE 0u
#define OPT_HELP 1u << 0u
#define OPT_FILTER 1u << 1u
#define OPT_FORCE 1u << 2u
#define OPT_RUNTIME 1u << 3u
#define OPT_NO_RELOAD 1u << 4u
#define OPT_WATCH 1u << 5u
#define OPT_KILL_WHOM 1u << 6u
#define OPT_SIGNAL 1u << 7u

int method_version(UNUSED Command *command, UNUSED void *userdata) {
        printf("bluechictl version %s\n", CONFIG_H_BC_VERSION);
        return 0;
}

const Method methods[] = {
        { "help",            0, 0,       OPT_NONE,                                method_help,               usage_bluechi                   },
        { "list-unit-files", 0, 1,       OPT_FILTER,                              method_list_unit_files,    usage_method_list_unit_files    },
        { "is-enabled",      2, 2,       OPT_NONE,                                method_is_enabled,         usage_method_is_enabled         },
        { "list-units",      0, 1,       OPT_FILTER,                              method_list_units,         usage_method_list_units         },
        { "start",           2, 2,       OPT_NONE,                                method_start,              usage_method_lifecycle          },
        { "stop",            2, 2,       OPT_NONE,                                method_stop,               usage_method_lifecycle          },
        { "freeze",          2, 2,       OPT_NONE,                                method_freeze,             usage_method_freeze             },
        { "thaw",            2, 2,       OPT_NONE,                                method_thaw,               usage_method_thaw               },
        { "restart",         2, 2,       OPT_NONE,                                method_restart,            usage_method_lifecycle          },
        { "reload",          2, 2,       OPT_NONE,                                method_reload,             usage_method_lifecycle          },
        { "reset-failed",    0, ARG_ANY, OPT_NONE,                                method_reset_failed,       usage_method_reset_failed       },
        { "kill",            2, 2,       OPT_KILL_WHOM | OPT_SIGNAL,              method_kill,               usage_method_kill               },
        { "monitor",         0, 2,       OPT_NONE,                                method_monitor,            usage_method_monitor            },
        { "metrics",         1, 1,       OPT_NONE,                                method_metrics,            usage_method_metrics            },
        { "enable",          2, ARG_ANY, OPT_FORCE | OPT_RUNTIME | OPT_NO_RELOAD, method_enable,             usage_method_enable             },
        { "disable",         2, ARG_ANY, OPT_NO_RELOAD,                           method_disable,            usage_method_disable            },
        { "daemon-reload",   1, 1,       OPT_NONE,                                method_daemon_reload,      usage_method_daemon_reload      },
        { "status",          0, ARG_ANY, OPT_WATCH,                               method_status,             usage_method_status             },
        { "set-loglevel",    1, 2,       OPT_NONE,                                method_set_loglevel,       usage_method_set_loglevel       },
        { "get-default",     1, 1,       OPT_NONE,                                method_get_default_target, usage_method_get_default_target },
        { "set-default",     2, 2,       OPT_NONE,                                method_set_default_target, usage_method_set_default_target },
        { "version",         0, 0,       OPT_NONE,                                method_version,            usage_bluechi                   },
        { NULL,              0, 0,       0,                                       NULL,                      NULL                            }
};

const OptionType option_types[] = {
        { ARG_FILTER_SHORT,    ARG_FILTER,    OPT_FILTER    },
        { ARG_FORCE_SHORT,     ARG_FORCE,     OPT_FORCE     },
        { ARG_RUNTIME_SHORT,   ARG_RUNTIME,   OPT_RUNTIME   },
        { ARG_NO_RELOAD_SHORT, ARG_NO_RELOAD, OPT_NO_RELOAD },
        { ARG_WATCH_SHORT,     ARG_WATCH,     OPT_WATCH     },
        { ARG_KILL_WHOM_SHORT, ARG_KILL_WHOM, OPT_KILL_WHOM },
        { ARG_SIGNAL_SHORT,    ARG_SIGNAL,    OPT_SIGNAL    },
        { 0,                   NULL,          0             }
};

#define GETOPT_OPTSTRING ARG_HELP_SHORT_S ARG_FORCE_SHORT_S ARG_WATCH_SHORT_S
const struct option getopt_options[] = {
        { ARG_HELP,      no_argument,       0, ARG_HELP_SHORT      },
        { ARG_FILTER,    required_argument, 0, ARG_FILTER_SHORT    },
        { ARG_FORCE,     no_argument,       0, ARG_FORCE_SHORT     },
        { ARG_RUNTIME,   no_argument,       0, ARG_RUNTIME_SHORT   },
        { ARG_NO_RELOAD, no_argument,       0, ARG_NO_RELOAD_SHORT },
        { ARG_WATCH,     no_argument,       0, ARG_WATCH_SHORT     },
        { ARG_KILL_WHOM, required_argument, 0, ARG_KILL_WHOM_SHORT },
        { ARG_SIGNAL,    required_argument, 0, ARG_SIGNAL_SHORT    },
        { NULL,          0,                 0, '\0'                }
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
