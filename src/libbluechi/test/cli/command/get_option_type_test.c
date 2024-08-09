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
        { 0, "zero", 1  },
        { 1, "one",  11 },
        { 0, NULL,   0  },
};

bool check_option_type(const OptionType *got, OptionType *expected) {
        if (got == NULL && expected != NULL) {
                fprintf(stderr,
                        "FAILED: Expected option type '%s' for get_option_type(types, %d), but got NULL\n",
                        expected->option_long,
                        expected->option_short);
                return false;
        }
        if (got != NULL && expected == NULL) {
                fprintf(stderr, "FAILED: Expected NULL for get_option_type, but got %s\n", got->option_long);
                return false;
        }

        if (got != NULL && expected != NULL && !streq(got->option_long, expected->option_long)) {
                fprintf(stderr,
                        "FAILED: Expected option type '%s' for get_option_type(types, %d), but got %s\n",
                        expected->option_long,
                        expected->option_short,
                        got->option_long);
                return false;
        }

        return true;
}

int main() {
        bool result = true;

        const OptionType *type = NULL;
        OptionType expected = option_types[0];
        type = get_option_type(option_types, 0);
        result = result && check_option_type(type, &expected);

        expected = option_types[1];
        type = get_option_type(option_types, 1);
        result = result && check_option_type(type, &expected);

        type = get_option_type(option_types, 2);
        result = result && check_option_type(type, NULL);

        type = get_option_type(option_types, -1);
        result = result && check_option_type(type, NULL);


        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
