/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdio.h>

void usage_print_header() {
        printf("bluechictl is a convenience CLI tool to interact with bluechi\n");
        printf("\n");
}

void usage_print_description(const char *description) {
        printf("Description: \n");
        printf("  %s\n", description);
        printf("\n");
}

void usage_print_usage(const char *usage) {
        printf("Usage: \n");
        printf("  %s\n", usage);
        printf("\n");
}
