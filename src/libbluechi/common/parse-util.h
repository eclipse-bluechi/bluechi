/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <inttypes.h>
#include <stdbool.h>

bool parse_long(const char *in, long *ret);
bool parse_port(const char *in, uint16_t *ret);

#define MIN_SIGNAL_VALUE 1
#define MAX_SIGNAL_VALUE 31

bool parse_linux_signal(const char *in, uint32_t *ret);
char *parse_selinux_type(const char *context);
