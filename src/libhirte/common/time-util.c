/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <time.h>

#include "time-util.h"

uint64_t get_time_micros() {
        struct timespec now;
        if (clock_gettime(CLOCK_REALTIME, &now) < 0) {
                return 0;
        }
        uint64_t now_micros = now.tv_sec * sec_to_microsec_multiplier +
                        (uint64_t) ((double) now.tv_nsec * nanosec_to_microsec_multiplier);
        return now_micros;
}

uint64_t finalize_time_interval_micros(int64_t start_time_micros) {
        return get_time_micros() - start_time_micros;
}

double micros_to_millis(uint64_t time_micros) {
        return (double) time_micros * microsec_to_millisec_multiplier;
}
