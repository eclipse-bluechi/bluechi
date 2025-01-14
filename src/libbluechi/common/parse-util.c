/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "parse-util.h"

bool parse_long(const char *in, long *ret) {
        const int base = 10;
        char *x = NULL;

        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        errno = 0;
        long r = strtol(in, &x, base);
        if (errno > 0 || !x || x == in || *x != 0) {
                return false;
        }
        *ret = r;

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

bool parse_linux_signal(const char *in, uint32_t *ret) {
        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        bool r = false;
        long l = 0;
        r = parse_long(in, &l);

        if (!r || l < MIN_SIGNAL_VALUE || l > MAX_SIGNAL_VALUE) {
                return false;
        }

        *ret = (uint32_t) l;
        return true;
}

char *parse_selinux_type(const char *context) {
        /* Format is user:role:type:level */

        /* Skip user */
        char *s = strchr(context, ':');
        if (s == NULL) {
                return NULL;
        }
        s++;
        if (*s == 0) {
                return NULL;
        }

        /* Skip role */
        s = strchr(s, ':');
        if (s == NULL) {
                return NULL;
        }
        s++;
        if (*s == 0) {
                return NULL;
        }

        char *end = strchr(s, ':');
        if (end == NULL) {
                return NULL;
        }

        return strndup(s, end - s);
}
