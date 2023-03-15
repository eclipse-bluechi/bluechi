/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool copy_str(char **p, const char *s) {
        char *dup = strdup(s);
        if (dup == NULL) {
                return false;
        }
        free(*p);
        *p = dup;
        return true;
}
