/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <assert.h>
#include <limits.h>
#include <stdlib.h>

#include "libbluechi/common/time-util.h"

int main() {
        struct timespec ts = { 0, 0 };

        assert(timespec_to_micros(&ts) == 0);

        ts.tv_sec = -1;
        ts.tv_nsec = 0;
        assert(timespec_to_micros(&ts) == USEC_INFINITY);

        ts.tv_sec = 0;
        ts.tv_nsec = -1;
        assert(timespec_to_micros(&ts) == USEC_INFINITY);

        ts.tv_sec = 0;
        ts.tv_nsec = 1234;
        assert(timespec_to_micros(&ts) == 1);

        ts.tv_sec = 5678;
        ts.tv_nsec = 0;
        assert(timespec_to_micros(&ts) == 5678000000);

        ts.tv_sec = 123;
        ts.tv_nsec = 4567890;
        assert(timespec_to_micros(&ts) == 123004567);

        if (LONG_MAX < UINT64_MAX / USEC_PER_SEC) {
                ts.tv_sec = LONG_MAX;
                ts.tv_nsec = 999999999;
                assert(timespec_to_micros(&ts) == (uint64_t) LONG_MAX * USEC_PER_SEC + 999999);
        }

#if defined(__x86_64__) || defined(__aarch64__)
        ts.tv_sec = LONG_MAX;
        ts.tv_nsec = 0;
        assert(timespec_to_micros(&ts) == USEC_INFINITY);

        ts.tv_sec = UINT64_MAX / USEC_PER_SEC;
        ts.tv_nsec = 0;
        assert(timespec_to_micros(&ts) == 18446744073709000000ULL);

        ts.tv_sec = UINT64_MAX / USEC_PER_SEC;
        ts.tv_nsec = 551614999;
        assert(timespec_to_micros(&ts) == 18446744073709551614ULL);

        ts.tv_sec = UINT64_MAX / USEC_PER_SEC;
        ts.tv_nsec = 551615000;
        assert(timespec_to_micros(&ts) == UINT64_MAX);

        ts.tv_sec = UINT64_MAX / USEC_PER_SEC;
        ts.tv_nsec = 551616000;
        assert(timespec_to_micros(&ts) == USEC_INFINITY);

        ts.tv_sec = UINT64_MAX / USEC_PER_SEC + 1;
        ts.tv_nsec = 0;
        assert(timespec_to_micros(&ts) == USEC_INFINITY);
#endif

        return EXIT_SUCCESS;
}
