/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/string-util.h"
#include "libbluechi/common/time-util.h"

bool test_get_log_timestamp(__time_t tv_sec, __time_t tv_nsec, char *expected_timestamp) {
        struct timespec time;
        time.tv_sec = tv_sec;
        time.tv_nsec = tv_nsec;
        _cleanup_free_ char *timestamp = get_formatted_log_timestamp_for_timespec(time, true);

        if (streq(timestamp, expected_timestamp)) {
                return true;
        }

        fprintf(stdout,
                "FAILED: for time (%ldsec, %ldnsec) - Expected %s, but got %s\n",
                tv_sec,
                tv_nsec,
                expected_timestamp,
                timestamp);
        return false;
}

int main() {
        bool result = true;

        result = result && test_get_log_timestamp(0, 0, "1970-01-01 00:00:00,000+0000");
        result = result && test_get_log_timestamp(1687075200, 0, "2023-06-18 08:00:00,000+0000");
        result = result && test_get_log_timestamp(1687075200, 999999999, "2023-06-18 08:00:00,999+0000");
        result = result && test_get_log_timestamp(1687075200, 999000000, "2023-06-18 08:00:00,999+0000");
        result = result && test_get_log_timestamp(1687075200, 998900000, "2023-06-18 08:00:00,998+0000");
        result = result && test_get_log_timestamp(1687046399, 999000000, "2023-06-17 23:59:59,999+0000");

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
