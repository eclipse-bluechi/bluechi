/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "libbluechi/common/common.h"
#include "time-util.h"

int get_time(struct timespec *now) {
        if (clock_gettime(CLOCK_REALTIME, now) < 0) {
                return -errno;
        }
        return 0;
}

uint64_t get_time_micros() {
        struct timespec now;
        if (get_time(&now) < 0) {
                return 0;
        }
        uint64_t now_micros = now.tv_sec * sec_to_microsec_multiplier +
                        (uint64_t) ((double) now.tv_nsec * nanosec_to_microsec_multiplier);
        return now_micros;
}

int get_time_seconds(time_t *ret_time_seconds) {
        if (ret_time_seconds == NULL) {
                return -EINVAL;
        }

        struct timespec now;
        int r = get_time(&now);
        if (r < 0) {
                return r;
        }
        *ret_time_seconds = now.tv_sec;
        return 0;
}

uint64_t finalize_time_interval_micros(int64_t start_time_micros) {
        return get_time_micros() - start_time_micros;
}

double micros_to_millis(uint64_t time_micros) {
        return (double) time_micros * microsec_to_millisec_multiplier;
}

char *get_formatted_log_timestamp() {
        struct timespec now;
        int r = get_time(&now);
        if (r < 0) {
                return NULL;
        }
        return get_formatted_log_timestamp_for_timespec(now, false);
}

char *get_formatted_log_timestamp_for_timespec(struct timespec time, bool is_gmt) {
        const size_t timestamp_size = 20;
        const size_t timestamp_with_millis_size = timestamp_size + 4;
        const size_t timestamp_offset_size = 7;
        const size_t timestamp_full_size = timestamp_with_millis_size + timestamp_offset_size;
        char timebuf[timestamp_size];
        char offsetbuf[timestamp_offset_size];
        char *timebuf_full = malloc0(timestamp_full_size);
        struct tm *time_tm = is_gmt ? gmtime(&time.tv_sec) : localtime(&time.tv_sec);

        strftime(timebuf, timestamp_size, "%Y-%m-%d %H:%M:%S", time_tm);
        uint64_t millis = (uint64_t) ((double) time.tv_nsec * nanosec_to_millisec_multiplier);
        strftime(offsetbuf, timestamp_offset_size, "%z", time_tm);
        snprintf(timebuf_full,
                 timestamp_full_size,
                 "%s,%03" PRIu64 "%s",
                 timebuf,
                 millis % millis_in_second,
                 offsetbuf);

        return timebuf_full;
}

bool time_lessp(struct timespec a, struct timespec b) {
        return a.tv_sec < b.tv_sec ||
                (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec);
}

bool time_elapsed_millis(struct timespec start, uint64_t duration, struct timespec now) {
        uint64_t secs = duration / 1000;
        uint64_t nsecs = (duration % 1000) * nanos_in_millis;

        /* This never wraps around, as tv_nsec is in [0, 1000000000) */
        start.tv_nsec += nsecs;
        secs += start.tv_nsec / nanos_in_second;
        start.tv_nsec %= nanos_in_second;

        if ((uint64_t) start.tv_sec < (uint64_t) now.tv_sec - secs) {
		return false;
	}
	/* This doesn't overflow, due to the if check above */
	start.tv_sec += secs;

	return !time_lessp(start, now);
}
