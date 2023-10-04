/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <inttypes.h>

int procfs_cpu_get_usage(uint64_t *ret);
int procfs_memory_get(uint64_t *ret_total, uint64_t *ret_used);
