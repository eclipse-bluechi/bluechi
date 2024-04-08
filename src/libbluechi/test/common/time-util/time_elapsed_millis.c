/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/string-util.h"
#include "libbluechi/common/time-util.h"

bool test_time_elapsed_millis(struct timespec start, uint64_t millis,
                              struct timespec now, bool expected_return) {
        bool r = time_elapsed_millis(start, millis, now);

        if (r == expected_return) {
                return true;
        }

        fprintf(stdout,
                "FAILED: time_elapsed_millis() - Expected return value %d, but got %d\n",
                expected_return,
                r);
        return false;
}

int main() {
        bool result = true;
        struct timespec start, now;

        start.tv_sec = 1;
        start.tv_nsec = 1000000000UL - 3000000UL;

        now.tv_sec = 2;
        now.tv_nsec = 0;

        result = result && test_time_elapsed_millis(start, 1, now, false);

        result = result && test_time_elapsed_millis(start, 2, now, false);

        result = result && test_time_elapsed_millis(start, 3, now, true);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
