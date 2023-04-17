/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define streq(a, b) (strcmp((a), (b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)

#define streqi(a, b) (strcasecmp(a, b) == 0)
#define strneqi(a, b, n) (strncasecmp(a, b, n) == 0)

static inline bool ascii_isdigit(char a) {
        return a >= '0' && a <= '9';
}

static inline bool ascii_isalpha(char a) {
        return (a >= 'a' && a <= 'z') || (a >= 'A' && a <= 'Z');
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

static inline bool copy_str(char **p, const char *s) {
        char *dup = strdup(s);
        if (dup == NULL) {
                return false;
        }
        free(*p);
        *p = dup;
        return true;
}
