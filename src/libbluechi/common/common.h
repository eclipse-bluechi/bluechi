/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "list.h"
#include "protocol.h"
#include "string-util.h"

#define UNUSED __attribute__((unused))

static inline void *steal_pointer(void *ptr) {
        void **ptrp = (void **) ptr;
        void *p = *ptrp;
        *ptrp = NULL;
        return p;
}

static inline int steal_fd(int *fdp) {
        int fd = *fdp;
        *fdp = -1;
        return fd;
}

static inline void closep(const int *fd) {
        if (*fd >= 0) {
                close(*fd);
        }
}

static inline void freep(void *p) {
        free(*(void **) p);
}

/* Free null terminated array of pointers (each with free) */
static inline void freev(void **p) {
        if (p) {
                for (size_t i = 0; p[i] != NULL; i++) {
                        free(p[i]);
                }
                free(p);
        }
}

static inline void freepv(void *p) {
        freev(*(void ***) p);
}


// NOLINTBEGIN(bugprone-macro-parentheses)
#define free_and_null(ptr) \
        free(ptr);         \
        ptr = NULL;
// NOLINTEND(bugprone-macro-parentheses)

typedef void (*free_func_t)(void *ptr);

#define malloc0(n) (calloc(1, (n) ?: 1))

static inline void *malloc0_array(size_t base_size, size_t element_size, size_t n_elements) {
        /* Check for overflow of multiplication */
        if (element_size > 0 && n_elements > SIZE_MAX / element_size) {
                return NULL;
        }

        /* Check for overflow of addition */
        size_t array_size = element_size * n_elements;
        size_t total_size = base_size + array_size;
        if (total_size < array_size) {
                return NULL;
        }

        return malloc0(total_size);
}

#define _cleanup_(x) __attribute__((__cleanup__(x)))
#define _cleanup_free_ _cleanup_(freep)
#define _cleanup_freev_ _cleanup_(freepv)
#define _cleanup_fd_ _cleanup_(closep)
#define _cleanup_string_builder_ _cleanup_(string_builder_destroy)

// NOLINTBEGIN(bugprone-macro-parentheses)
#define DEFINE_CLEANUP_FUNC(_type, _freefunc)         \
        static inline void _freefunc##p(_type **pp) { \
                if (pp && *pp) {                      \
                        _freefunc(*pp);               \
                        *pp = NULL;                   \
                }                                     \
        }
// NOLINTEND(bugprone-macro-parentheses)

#define _cleanup_sd_event_ _cleanup_(sd_event_unrefp)
#define _cleanup_sd_event_source_ _cleanup_(sd_event_source_unrefp)
#define _cleanup_sd_bus_ _cleanup_(sd_bus_unrefp)
#define _cleanup_sd_bus_slot_ _cleanup_(sd_bus_slot_unrefp)
#define _cleanup_sd_bus_message_ _cleanup_(sd_bus_message_unrefp)
#define _cleanup_sd_bus_error_ _cleanup_(sd_bus_error_free)
