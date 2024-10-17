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
        { 't', "timeout", 1  },
        { 'r', "reload",  11 },
        { 0,   NULL,      0  },
};

bool test_command_get_option_long_success() {
        _cleanup_command_ Command *command = new_command();

        int r = command_add_option(command, 't', "1000", &option_types[0]);
        if (r < 0) {
                fprintf(stderr, "FAILED: Unexpected failure when adding option 't': %s\n", strerror(-r));
                return false;
        }

        long got = 0;
        r = command_get_option_long(command, 't', &got);
        if (r < 0) {
                fprintf(stderr, "FAILED: Getting option 't' with value '1000' failed with: %s\n", strerror(-r));
                return false;
        }

        return got == 1000;
}

bool test_command_get_option_long_invalid_value() {
        _cleanup_command_ Command *command = new_command();

        int r = command_add_option(command, 't', "10p", &option_types[0]);
        if (r < 0) {
                fprintf(stderr, "FAILED: Unexpected failure when adding option 't': %s\n", strerror(-r));
                return false;
        }

        long got = 0;
        r = command_get_option_long(command, 't', &got);
        if (r != -EINVAL) {
                fprintf(stderr,
                        "FAILED: Getting option 't' with value '10p' should return EINVAL, but didn't");
                return false;
        }

        // got should not be changed
        return got == 0;
}

int main() {
        bool result = true;
        result = result && test_command_get_option_long_success();
        result = result && test_command_get_option_long_invalid_value();

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
