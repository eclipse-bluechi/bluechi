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

        /* adding command option without value */
        int r = command_add_option(command, 'f', NULL, &option_types[0]);
        if (r < 0) {
                fprintf(stderr, "FAILED: Unexpected error adding command option: %s\n", strerror(-r));
                return EXIT_FAILURE;
        }

        /* adding same command option with value */
        r = command_add_option(command, 'f', "value", &option_types[0]);
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
         * Check command_flag_exists finds added options and test if
         */

        if (!command_flag_exists(command, 'f')) {
                fprintf(stderr, "FAILED: Expected flag 'f' to exist, but couldn't find it\n");
                result = false;
        }
        if (!command_flag_exists(command, 'r')) {
                fprintf(stderr, "FAILED: Expected flag 'r' to exist, but couldn't find it\n");
                result = false;
        }
        if (command_flag_exists(command, 'x')) {
                fprintf(stderr, "FAILED: Found unexpected flag 'x'\n");
                result = false;
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
