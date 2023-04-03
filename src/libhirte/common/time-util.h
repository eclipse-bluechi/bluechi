/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <inttypes.h>

static const uint64_t sec_to_microsec_multiplier = 1000000;
static const double microsec_to_millisec_multiplier = 1e-3;
static const double nanosec_to_microsec_multiplier = 1e-3;

uint64_t get_time_micros();
uint64_t finalize_time_interval_micros(int64_t start_time_micros);
double micros_to_millis(uint64_t time_micros);
