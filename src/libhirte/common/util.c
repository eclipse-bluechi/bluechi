#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "util.h"

bool parse_long(const char *in, long *ret) {
        const int base = 10;
        char *x = NULL;

        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        errno = 0;
        *ret = strtol(in, &x, base);

        if (errno > 0 || !x || x == in || *x != 0) {
                return false;
        }

        return true;
}

bool parse_port(const char *in, uint16_t *ret) {
        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        bool r = false;
        long l = 0;
        r = parse_long(in, &l);

        if (!r || l < 0 || l > USHRT_MAX) {
                return false;
        }

        *ret = (uint16_t) l;
        return true;
}

int64_t get_time_millis() {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        int64_t now_millis = now.tv_sec*1000 + now.tv_nsec*1e-6;
        return now_millis;
}

int64_t finalize_time_interval_millis(int64_t start_time_millis) {
        return get_time_millis() - start_time_millis;
}
