#pragma once

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "list.h"
#include "protocol.h"

#define streq(a, b) (strcmp((a), (b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)

#define streqi(a, b) (strcasecmp(a, b) == 0)
#define strneqi(a, b) (strncasecmp(a, b) == 0)

static inline bool ascii_isdigit(char a) {
        return a >= '0' && a <= '9';
}

static inline bool ascii_isalpha(char a) {
        return (a >= 'a' && a <= 'z') || (a >= 'A' && a <= 'Z');
}

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

static inline char *strcat_dup(const char *a, const char *b) {
        size_t a_len = strlen(a);
        size_t b_len = strlen(b);
        size_t len = a_len + b_len + 1;
        char *res = malloc(len);
        if (res) {
                memcpy(res, a, a_len);
                memcpy(res + a_len, b, b_len);
                res[a_len + b_len] = 0;
        }

        return res;
}

#define _cleanup_(x) __attribute__((__cleanup__(x)))
#define _cleanup_free_ _cleanup_(freep)
#define _cleanup_fd_ _cleanup_(closep)

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
