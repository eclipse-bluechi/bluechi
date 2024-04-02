/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/cli/command.h"


static int method_dummy_call_count = 0;
static int method_dummy(UNUSED Command *command, UNUSED void *userdata) {
        method_dummy_call_count++;
        return 0;
}

static int usage_dummy_call_count = 0;
static void usage_dummy() {
        usage_dummy_call_count++;
}


bool test_command_execute_is_help() {
        usage_dummy_call_count = 0;
        method_dummy_call_count = 0;

        const Method method = { "dummy", 0, 0, 0, method_dummy, usage_dummy };

        _cleanup_command_ Command *command = new_command();
        command->is_help = true;
        command->method = &method;

        bool result = true;
        int r = command_execute(command, NULL);
        if (r != 0) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute for help method return 0, but got %d\n",
                        __func__,
                        r);
                result = false;
        }
        if (usage_dummy_call_count != 1) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute to call usage(), but did not\n",
                        __func__);
                result = false;
        }

        return result;
}

bool test_command_execute_min_max_args() {
        usage_dummy_call_count = 0;
        method_dummy_call_count = 0;

        // define method with min_args=1 and max_args=2
        const Method method = { "dummy", 1, 2, 0, method_dummy, usage_dummy };

        _cleanup_command_ Command *command = new_command();
        command->is_help = false;
        command->method = &method;

        bool result = true;

        command->opargc = 0;
        int r = command_execute(command, NULL);
        if (r != -EINVAL) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute with too few arguments to return %d, but got %d\n",
                        __func__,
                        -EINVAL,
                        r);
                result = false;
        }

        command->opargc = 3;
        r = command_execute(command, NULL);
        if (r != -EINVAL) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute with too many arguments to return %d, but got %d\n",
                        __func__,
                        -EINVAL,
                        r);
                result = false;
        }

        if (usage_dummy_call_count != 0) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute not to call usage(), but did\n",
                        __func__);
                result = false;
        }
        if (method_dummy_call_count != 0) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute not to call dispatch(), but did\n",
                        __func__);
                result = false;
        }

        return result;
}

bool test_command_execute_dispatch() {
        usage_dummy_call_count = 0;
        method_dummy_call_count = 0;

        const Method method = { "dummy", 0, 0, 0, method_dummy, usage_dummy };

        _cleanup_command_ Command *command = new_command();
        command->is_help = false;
        command->opargc = 0;
        command->method = &method;

        bool result = true;
        int r = command_execute(command, NULL);
        if (r != 0) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute to return %d on dispatch(), but got %d\n",
                        __func__,
                        0,
                        r);
        }
        if (usage_dummy_call_count != 0) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute not to call usage(), but did\n",
                        __func__);
                result = false;
        }
        if (method_dummy_call_count != 1) {
                fprintf(stderr,
                        "FAILED (%s): Expected command_execute to call dispatch() exactly once, but called it %d times\n",
                        __func__,
                        method_dummy_call_count);
                result = false;
        }

        return result;
}

int main() {
        bool result = true;

        result = result && test_command_execute_is_help();
        result = result && test_command_execute_min_max_args();
        result = result && test_command_execute_dispatch();

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
