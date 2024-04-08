/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

static const uint64_t sec_to_microsec_multiplier = 1000000;
static const double microsec_to_millisec_multiplier = 1e-3;
static const double nanosec_to_microsec_multiplier = 1e-3;
static const double nanosec_to_millisec_multiplier = 1e-6;
static const uint64_t millis_in_second = 1000;
static const uint64_t nanos_in_millis = 1000000;
static const uint64_t nanos_in_second = 1000000000;

int get_time(struct timespec *now);
uint64_t get_time_micros();
int get_time_seconds(time_t *ret_time_seconds);
uint64_t finalize_time_interval_micros(int64_t start_time_micros);
double micros_to_millis(uint64_t time_micros);
char *get_formatted_log_timestamp();
char *get_formatted_log_timestamp_for_timespec(struct timespec time, bool is_gmt);
bool time_lessp(struct timespec a, struct timespec b);
bool time_elapsed_millis(struct timespec start, uint64_t millis, struct timespec now);
