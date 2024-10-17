/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "help.h"
#include "opt.h"

static void usage_print_header() {
        printf("bluechi-is-online checks and monitors the connection state of BlueChi components\n");
        printf("\n");
}

static void usage_print_usage(const char *usage) {
        printf("Usage: \n");
        printf("  %s\n", usage);
        printf("\n");
}


void usage() {
        usage_print_header();
        usage_print_usage("bluechi-is-online [agent|node|system] [OPTIONS]");
        printf("Available commands:\n");
        printf("  help: \t shows this help message\n");
        printf("  version: \t shows the version of bluechi-is-online\n");
        printf("  agent: \t on the agent machine check and/or monitor the connection of the agent to the controller\n");
        printf("  node [name]: \t on the controller machine check and/or monitor the connection of the given node to the controller\n");
        printf("  system: \t on the controller machine check and/or monitor the connection of all nodes to the controller\n");
        printf("Available options:\n");
        printf("  --%s: \t\t keeps monitoring as long as [agent|node|system] is online and exits if it detects an offline state.\n",
               ARG_MONITOR);
        printf("  --%s: \t\t Wait n milliseconds for [agent|node|system] to get online.\n", ARG_WAIT);
        printf("  --%s: \t Only for [agent]. Wait n milliseconds for after switching controller to get online again.\n",
               ARG_SWITCH_CTRL_TIMEOUT);
}

int method_help(UNUSED Command *command, UNUSED void *userdata) {
        usage();
        return 0;
}
