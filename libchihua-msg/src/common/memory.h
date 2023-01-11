#pragma once

#include <stdlib.h>
#include <unistd.h>

#include "../../include/common/memory.h"

static inline void *steal_pointer(void *ptr) {
        void **ptrp = (void **) ptr;
        void *p = *ptrp;
        *ptrp = NULL;
        return p;
}

static inline void closep(const int *fd) {
        if (*fd >= 0) {
                close(*fd);
        }
}

static inline void freep(void *p) {
        free(*(void **) p);
}

#define malloc0(n) (calloc(1, (n) ?: 1))

#define _cleanup_free_ _cleanup_(freep)
#define _cleanup_fd_ _cleanup_(closep)
