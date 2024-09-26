/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-help.h"
#include "client.h"
#include "usage.h"

void usage_bluechi() {
        usage_print_header();
        usage_print_usage("bluechictl [command]");
        printf("Available commands:\n");
        printf("  - help: shows this help message\n");
        printf("    usage: help\n");
        printf("  - list-unit-files: returns the list of systemd service files installed on a specific or on all nodes\n");
        printf("    usage: list-unit-files [nodename] [--filter=glob]\n");
        printf("  - list-units: returns the list of systemd services running on a specific or on all nodes\n");
        printf("    usage: list-units [nodename] [--filter=glob]\n");
        printf("  - start: starts a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: start nodename unitname\n");
        printf("  - stop: stop a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: stop nodename unitname\n");
        printf("  - freeze: freeze a specific systemd service on a specific node\n");
        printf("    usage: freeze nodename unitname\n");
        printf("  - thaw: thaw a frozen systemd service on a specific node\n");
        printf("    usage: thaw nodename unitname\n");
        printf("  - reload: reloads a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: reload nodename unitname\n");
        printf("  - restart: restarts a specific systemd service (or timer, or slice) on a specific node\n");
        printf("    usage: restart nodename unitname\n");
        printf("  - reset-failed: reset failed node on all node or all units on a specific node or specidec units on a node \n");
        printf("    usage: reset-failed [nodename] [unit1, unit2 ...]\n");
        printf("  - kill: kills processes of a specific unit on the given node\n");
        printf("    usage: kill [nodename] [unit] [--kill-whom=all|main|control] [--signal=<number>]\n");
        printf("  - enable: enables the specified systemd files on a specific node\n");
        printf("    usage: enable nodename unitfilename...\n");
        printf("  - disable: disables the specified systemd files on a specific node\n");
        printf("    usage: disable nodename unitfilename...\n");
        printf("  - metrics [enable|disable]: enables/disables metrics reporting\n");
        printf("    usage: metrics [enable|disable]\n");
        printf("  - metrics listen: listen and print incoming metrics reports\n");
        printf("    usage: metrics listen\n");
        printf("  - monitor: creates a monitor on the given node to observe changes in the specified units\n");
        printf("    usage: monitor [node] [unit1,unit2,...]\n");
        printf("  - status: shows the status of a node, or statuses of all nodes, or status of a unit on node\n");
        printf("    usage: status [nodename [unitname]] [-w/--watch]\n");
        printf("  - daemon-reload: reload systemd daemon on a specific node\n");
        printf("    usage: daemon-reload nodename\n");
        printf("  - set-loglevel: change the log level of the bluechi-controller or a connected node\n");
        printf("    usage: set-loglevel [nodename] [loglevel]\n");
        printf("  - get-default: get a default target of connected node\n");
        printf("    usage: get-default [nodename]\n");
        printf("  - set-default: set a default target of connected node\n");
        printf("    usage: set-default [nodename] [target]\n");
}

int method_help(UNUSED Command *command, UNUSED void *userdata) {
        usage_bluechi();
        return 0;
}
