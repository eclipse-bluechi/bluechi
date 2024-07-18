/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "method-help.h"
#include "client.h"

void usage_bluechi() {
        printf("bluechictl is a convenience CLI tool to interact with bluechi\n");
        printf("Usage: bluechictl COMMAND\n\n");
        printf("Available command:\n");
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
}

int method_help(UNUSED Command *command, UNUSED void *userdata) {
        usage_bluechi();
        return 0;
}
