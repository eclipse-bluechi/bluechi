/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libbluechi/bus/bus.h"
#include "libbluechi/common/common.h"
#include "libbluechi/common/opt.h"
#include "libbluechi/log/log.h"

const struct option options[] = {
        { ARG_USER,    no_argument, 0, ARG_USER_SHORT    },
        { ARG_HELP,    no_argument, 0, ARG_HELP_SHORT    },
        { ARG_VERSION, no_argument, 0, ARG_VERSION_SHORT },
        { NULL,        0,           0, '\0'              }
};

#define GETOPT_OPTSTRING ARG_HELP_SHORT_S ARG_USER_SHORT_S ARG_VERSION_SHORT_S

static const char *opt_node_unit = NULL;
static const char *opt_operation = NULL;
static bool opt_user = false;

#ifdef USE_USER_API_BUS
#        define ALWAYS_USER_API_BUS 1
#else
#        define ALWAYS_USER_API_BUS 0
#endif

#define OPT_OPERATION_CREATE "create"
#define OPT_OPERATION_REMOVE "remove"

static void usage(char *argv[]) {
        printf("Usage: %s \n"
               "\t[-h help]\t\t Print this help message.\n"
               "\t<operation>\t\t Required. Operation to perform. Must be one of [%s,%s].\n"
               "\t<node>_<service>\t Required. Input the name of the node the required service is supposed to run on.\n"
               "\t<[-v version]\t Print current version of bluechi-proxy\n",
               argv[0],
               OPT_OPERATION_CREATE,
               OPT_OPERATION_REMOVE);
}

int parse_node_unit_opt(const char *opt_node_unit, char **ret_node_name, char **ret_unit_name) {
        if (opt_node_unit == NULL || ret_node_name == NULL || ret_unit_name == NULL) {
                return -EINVAL;
        }

        char *split = strchr(opt_node_unit, '_');
        if (split == NULL || split == opt_node_unit) {
                fprintf(stderr, "No underscore in unit name '%s'\n", opt_node_unit);
                return -EINVAL;
        }
        if (split[1] == 0) {
                fprintf(stderr, "No service specified in unit name '%s'\n", opt_node_unit);
                return -EINVAL;
        }

        *ret_node_name = strndup(opt_node_unit, split - opt_node_unit);
        *ret_unit_name = strdup(split + 1);

        if (*ret_node_name == NULL || *ret_unit_name == NULL) {
                fprintf(stderr, "Out of memory\n");
                return -ENOMEM;
        }

        return 0;
}

static int get_opts(int argc, char *argv[]) {
        int opt = 0;
        while ((opt = getopt_long(argc, argv, GETOPT_OPTSTRING, options, NULL)) != -1) {
                switch (opt) {
                case ARG_HELP_SHORT:
                        usage(argv);
                        return 1;

                case ARG_VERSION_SHORT:
                        printf("bluechi-proxy version %s\n", CONFIG_H_BC_VERSION);
                        return 1;

                case ARG_USER_SHORT:
                        opt_user = true;
                        break;
                default:
                        fprintf(stderr, "Unsupported option %c\n", opt);
                        usage(argv);
                        return -EINVAL;
                }
        }

        if (argc < 3) {
                fprintf(stderr, "Not enough arguments specified\n");
                return -EINVAL;
        }
        opt_operation = argv[optind];
        opt_node_unit = argv[optind + 1];

        if (!streqi(opt_operation, OPT_OPERATION_CREATE) && !streqi(opt_operation, OPT_OPERATION_REMOVE)) {
                fprintf(stderr, "Unsupported command %s\n", opt_operation);
                return -EINVAL;
        }

        return 0;
}


static int call_proxy_method(
                sd_bus *api_bus,
                const char *method,
                const char *service_name,
                const char *node_name,
                const char *unit_name) {
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *message = NULL;
        _cleanup_free_ char *proxy_service = strcat_dup("bluechi-proxy@", service_name);

        int r = sd_bus_call_method(
                        api_bus,
                        BC_AGENT_DBUS_NAME,
                        BC_AGENT_OBJECT_PATH,
                        AGENT_INTERFACE,
                        method,
                        &error,
                        &message,
                        "sss",
                        proxy_service,
                        node_name,
                        unit_name);
        if (r < 0) {
                if (streq(error.name, BC_BUS_ERROR_ACTIVATION_FAILED)) {
                        fprintf(stderr,
                                "Failed to activate %s (on %s): %s\n",
                                proxy_service,
                                node_name,
                                error.message);
                } else {
                        fprintf(stderr, "Failed to call %s: %s\n", method, error.message);
                }
                return r;
        }
        return 0;
}

int main(int argc, char *argv[]) {
        bc_log_set_quiet(false);
        bc_log_set_level(LOG_LEVEL_INFO);
        bc_log_set_log_fn(bc_log_to_stderr_with_location);

        int r = get_opts(argc, argv);
        if (r < 0) {
                return EXIT_FAILURE;
        } else if (r > 0) {
                return EXIT_SUCCESS;
        }

        _cleanup_free_ char *node_name = NULL;
        _cleanup_free_ char *unit_name = NULL;
        parse_node_unit_opt(opt_node_unit, &node_name, &unit_name);

        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s", strerror(-r));
                return EXIT_FAILURE;
        }

        _cleanup_sd_bus_ sd_bus *api_bus = NULL;
        if (opt_user || ALWAYS_USER_API_BUS) {
                api_bus = user_bus_open(event);
        } else {
                api_bus = system_bus_open(event);
        }
        if (api_bus == NULL) {
                bc_log_error("Failed to create api bus");
                return EXIT_FAILURE;
        }

        /* Don't time out in the proxy calls, we want to wait until the target service is running */
        sd_bus_set_method_call_timeout(api_bus, UINT64_MAX);

        /* lint complains due to passing NULL. check for NULL done in other func. Disabling it. */
        // NOLINTNEXTLINE
        if (streqi(opt_operation, OPT_OPERATION_CREATE)) {
                r = call_proxy_method(api_bus, "CreateProxy", opt_node_unit, node_name, unit_name);
        } else if (streqi(opt_operation, OPT_OPERATION_REMOVE)) {
                r = call_proxy_method(api_bus, "RemoveProxy", opt_node_unit, node_name, unit_name);
        }

        if (r < 0) {
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}
