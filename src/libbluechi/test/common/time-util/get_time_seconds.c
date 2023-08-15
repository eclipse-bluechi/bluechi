/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/string-util.h"
#include "libbluechi/common/time-util.h"

bool test_get_time_seconds(time_t *input, int expected_return) {
        int r = get_time_seconds(input);

        if (r == expected_return) {
                return true;
        }

        fprintf(stdout,
                "FAILED: get_time_seconds() - Expected return value %d (%s), but got %d (%s)\n",
                expected_return,
                strerror(-expected_return),
                r,
                strerror(-r));
        return false;
}

int main() {
        bool result = true;

        result = result && test_get_time_seconds(NULL, -EINVAL);

        time_t ret = 0;
        result = result && test_get_time_seconds(&ret, 0);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
