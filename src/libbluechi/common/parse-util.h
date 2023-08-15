/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <inttypes.h>
#include <stdbool.h>

bool parse_long(const char *in, long *ret);
bool parse_port(const char *in, uint16_t *ret);
