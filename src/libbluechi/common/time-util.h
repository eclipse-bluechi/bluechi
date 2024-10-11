/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#define USEC_INFINITY UINT64_MAX
#define USEC_PER_SEC 1000000
#define USEC_PER_MSEC 1000
#define NSEC_PER_USEC 1000

static const uint64_t sec_to_microsec_multiplier = 1000000;
static const double microsec_to_millisec_multiplier = 1e-3;
static const double nanosec_to_microsec_multiplier = 1e-3;
static const double nanosec_to_millisec_multiplier = 1e-6;
static const uint64_t millis_in_second = 1000;

uint64_t get_time_micros();
uint64_t get_time_micros_monotonic();
uint64_t finalize_time_interval_micros(int64_t start_time_micros);
double micros_to_millis(uint64_t time_micros);
uint64_t timespec_to_micros(const struct timespec *ts);
char *get_formatted_log_timestamp();
char *get_formatted_log_timestamp_for_timespec(struct timespec time, bool is_gmt);
