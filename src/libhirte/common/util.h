#pragma once

#include <inttypes.h>
#include <stdbool.h>

bool parse_long(const char *in, long *ret);
bool parse_port(const char *in, uint16_t *ret);
long get_time_millis();
int64_t finalize_time_interval_millis(int64_t start_time_millis);
