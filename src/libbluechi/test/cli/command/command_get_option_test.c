/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/cli/command.h"


static const OptionType option_types[] = {
        { 'f', "force",  1  },
        { 'r', "reload", 11 },
        { 0,   NULL,     0  },
};

int main() {
        bool result = true;

        _cleanup_command_ Command *command = new_command();

        /* adding same command option with value */
        int r = command_add_option(command, 'f', "yes", &option_types[0]);
        if (r < 0) {
                fprintf(stderr, "FAILED: Unexpected error adding command option: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        /* adding different command option without value */
        r = command_add_option(command, 'r', NULL, &option_types[1]);
        if (r < 0) {
                fprintf(stderr, "FAILED: Unexpected error adding command option: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        /* Test validation:
         * Check command_get_option get the value of an option if found by key
         */

        char *got = command_get_option(command, 'f');
        if (got == NULL || !streq(got, "yes")) {
                fprintf(stderr, "FAILED: Expected value 'yes' for flag 'f', but got '%s'\n", got);
                result = false;
        }
        got = command_get_option(command, 'r');
        if (got != NULL) {
                fprintf(stderr, "FAILED: Expected NULL value for flag 'r', but got '%s'\n", got);
                result = false;
        }
        got = command_get_option(command, 'x');
        if (got != NULL) {
                fprintf(stderr, "FAILED: Expected NULL value for unknown flag 'x', but got '%s'\n", got);
                result = false;
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
