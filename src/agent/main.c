/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libbluechi/common/opt.h"
#include "libbluechi/log/log.h"

#include "agent.h"

const struct option options[] = { { ARG_HOST, required_argument, 0, ARG_HOST_SHORT },
                                  { ARG_PORT, required_argument, 0, ARG_PORT_SHORT },
                                  { ARG_ADDRESS, required_argument, 0, ARG_ADDRESS_SHORT },
                                  { ARG_NAME, required_argument, 0, ARG_NAME_SHORT },
                                  { ARG_HEARTBEAT_INTERVAL, required_argument, 0, ARG_HEARTBEAT_INTERVAL_SHORT },
                                  { ARG_CONFIG, required_argument, 0, ARG_CONFIG_SHORT },
                                  { ARG_USER, no_argument, 0, ARG_USER_SHORT },
                                  { ARG_HELP, no_argument, 0, ARG_HELP_SHORT },
                                  { ARG_VERSION, no_argument, 0, ARG_VERSION_SHORT },
                                  { NULL, 0, 0, '\0' } };

#define OPTIONS_STR                                                                               \
        ARG_PORT_SHORT_S ARG_HOST_SHORT_S ARG_ADDRESS_SHORT_S ARG_HELP_SHORT_S ARG_CONFIG_SHORT_S \
                        ARG_NAME_SHORT_S ARG_USER_SHORT_S ARG_HEARTBEAT_INTERVAL_SHORT_S ARG_VERSION_SHORT_S

static const char *opt_port = 0;
static const char *opt_host = NULL;
static const char *opt_address = NULL;
static const char *opt_name = NULL;
static const char *opt_config = NULL;
static bool opt_user = false;
static const char *opt_heartbeat_interval = 0;

static void usage(char *argv[]) {
        printf("Usage:\n"
               "\t%s [options...] \n"
               "Available options are:\n"
               "\t-%c %s\t\t Print this help message.\n"
               "\t-%c %s\t\t The host of bluechi to connect to. Must be a valid IPv4 or IPv6.\n"
               "\t-%c %s\t\t The port of bluechi to connect to.\n"
               "\t-%c %s\t The DBus address of bluechi to connect to. Overrides host and port.\n"
               "\t-%c %s\t\t The unique name of this bluechi-agent used to register at bluechi.\n"
               "\t-%c %s\t The interval between two heartbeat signals in milliseconds.\n"
               "\t-%c %s\t A path to a config file used to bootstrap bluechi-agent.\n"
               "\t-%c %s\t\t Connect to the user systemd instance instead of the system one.\n"
               "\t-%c %s\t\t Print current version of bluechi-agent.\n",
               argv[0],
               ARG_HELP_SHORT,
               ARG_HELP,
               ARG_HOST_SHORT,
               ARG_HOST,
               ARG_PORT_SHORT,
               ARG_PORT,
               ARG_ADDRESS_SHORT,
               ARG_ADDRESS,
               ARG_NAME_SHORT,
               ARG_NAME,
               ARG_HEARTBEAT_INTERVAL_SHORT,
               ARG_HEARTBEAT_INTERVAL,
               ARG_CONFIG_SHORT,
               ARG_CONFIG,
               ARG_USER_SHORT,
               ARG_USER,
               ARG_VERSION_SHORT,
               ARG_VERSION);
}

static int get_opts(int argc, char *argv[]) {
        int opt = 0;

        while ((opt = getopt_long(argc, argv, OPTIONS_STR, options, NULL)) != -1) {
                switch (opt) {
                case ARG_HELP_SHORT:
                        usage(argv);
                        return 1;

                case ARG_VERSION_SHORT:
                        printf("%s\n", CONFIG_H_BC_VERSION);
                        return 1;

                case ARG_NAME_SHORT:
                        opt_name = optarg;
                        break;

                case ARG_HOST_SHORT:
                        opt_host = optarg;
                        break;

                case ARG_ADDRESS_SHORT:
                        opt_address = optarg;
                        break;

                case ARG_HEARTBEAT_INTERVAL_SHORT:
                        opt_heartbeat_interval = optarg;
                        break;

                case ARG_CONFIG_SHORT:
                        opt_config = optarg;
                        break;

                case ARG_PORT_SHORT:
                        opt_port = optarg;
                        break;

                case ARG_USER_SHORT:
                        opt_user = true;
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

        _cleanup_agent_ Agent *agent = agent_new();
        if (agent == NULL) {
                return EXIT_FAILURE;
        }

        /* First load config */
        if (!agent_parse_config(agent, opt_config)) {
                return EXIT_FAILURE;
        }

        /* Then override individual options */
        agent_set_systemd_user(agent, opt_user);

        if (opt_port && !agent_set_port(agent, opt_port)) {
                return EXIT_FAILURE;
        }

        if (opt_host && !agent_set_host(agent, opt_host)) {
                return EXIT_FAILURE;
        }

        if (opt_address && !agent_set_orch_address(agent, opt_address)) {
                return EXIT_FAILURE;
        }

        if (opt_name && !agent_set_name(agent, opt_name)) {
                return EXIT_FAILURE;
        }

        if (opt_heartbeat_interval && !agent_set_heartbeat_interval(agent, opt_heartbeat_interval)) {
                return EXIT_FAILURE;
        }

        if (agent_start(agent)) {
                return EXIT_SUCCESS;
        }

        agent_stop(agent);
        return EXIT_SUCCESS;
}
