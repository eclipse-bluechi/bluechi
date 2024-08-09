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
         * Check that command options have been added without other functions
         * from command.h
         */

        int option_count = 0;
        int expected_option_count = 3;

        CommandOption *curr = NULL;
        CommandOption *next = NULL;
        LIST_FOREACH_SAFE(options, curr, next, command->command_options) {
                // first command found
                if (curr->key == 'f' && curr->value == NULL && curr->is_flag) {
                        option_count++;
                }
                // second command found
                else if (curr->key == 'f' && curr->value != NULL && streq(curr->value, "value") &&
                         !curr->is_flag) {
                        option_count++;
                }
                // third command found
                else if (curr->key == 'r' && curr->value == NULL && curr->is_flag) {
                        option_count++;
                } else {
                        fprintf(stderr, "FAILED: Found unexpected command option with key: %d\n", curr->key);
                        result = false;
                }
        }

        if (option_count != expected_option_count) {
                fprintf(stderr, "FAILED: Missing command options detected\n");
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
