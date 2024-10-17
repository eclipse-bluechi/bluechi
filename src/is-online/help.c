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
        printf("Available options:\n");
        printf("  --%s: \t keeps monitoring as long as [agent|node|system] is online and exits if it detects an offline state.\n",
               ARG_MONITOR);
        printf("  --%s: \t Wait n seconds for [agent|node|system] to get online.\n", ARG_WAIT);
}

int method_help(UNUSED Command *command, UNUSED void *userdata) {
        usage();
        return 0;
}
