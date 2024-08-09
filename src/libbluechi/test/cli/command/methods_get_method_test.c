/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/cli/command.h"


static int method_dummy_one(UNUSED Command *command, UNUSED void *userdata) {
        return 0;
}

static int method_dummy_two(UNUSED Command *command, UNUSED void *userdata) {
        return 0;
}

static void usage_dummy() {
}

static const Method methods[] = {
        { "dummy-one", 0, 0, 0, method_dummy_one, usage_dummy },
        { "dummy-two", 0, 0, 0, method_dummy_two, usage_dummy },
        { NULL,        0, 0, 0, NULL,             NULL        }
};

int main() {
        bool result = true;

        const Method *m = NULL;
        m = methods_get_method(NULL, methods);
        if (m != NULL) {
                result = false;
                fprintf(stderr, "FAILED: Expected methods_get_method(NULL, methods) to return NULL\n");
        }

        m = methods_get_method("dummy-one", methods);
        if (m == NULL || m->dispatch != method_dummy_one) {
                result = false;
                fprintf(stderr,
                        "FAILED: Expected methods_get_method('dummy-one', methods) to return method pointing to matching Method\n");
        }

        m = methods_get_method("dummy-two", methods);
        if (m == NULL || m->dispatch != method_dummy_two) {
                result = false;
                fprintf(stderr,
                        "FAILED: Expected methods_get_method('dummy-two', methods) to return  method pointing to matching Method\n");
        }

        m = methods_get_method("dumy-two", methods);
        if (m != NULL) {
                result = false;
                fprintf(stderr, "FAILED: Expected methods_get_method(NULL, methods) to return NULL\n");
        }


        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
