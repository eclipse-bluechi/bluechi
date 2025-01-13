/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <stdio.h>

#include "string-util.h"

bool string_builder_init(StringBuilder *builder, size_t initial_capacity) {
        if (initial_capacity == 0) {
                initial_capacity = STRING_BUILDER_DEFAULT_CAPACITY;
        }

        char *str = malloc(initial_capacity);
        if (str == NULL) {
                return false;
        }

        str[0] = 0; /* zero terminate */
        builder->str = str;
        builder->len = 0;
        builder->capacity = initial_capacity;

        return true;
}

void string_builder_destroy(StringBuilder *builder) {
        free(builder->str);
}

static bool string_builder_realloc(StringBuilder *builder, size_t new_size) {
        char *new_str = realloc(builder->str, new_size);
        if (new_str == NULL) {
                return false;
        }

        builder->str = new_str;
        builder->capacity = new_size;

        return true;
}

static bool string_builder_grow(StringBuilder *builder, size_t extra_size) {
        size_t required_size = builder->len + extra_size + 1;
        if (builder->capacity < required_size) {
                size_t new_size = builder->capacity * 2;
                if (new_size < required_size) {
                        new_size = required_size;
                }

                if (!string_builder_realloc(builder, new_size)) {
                        return false;
                }
        }

        builder->len += extra_size;
        return true;
}

bool string_builder_append(StringBuilder *builder, const char *str) {
        size_t old_len = builder->len;
        if (!string_builder_grow(builder, strlen(str))) {
                return false;
        }

        strncpy(builder->str + old_len, str, builder->capacity - old_len);

        return true;
}

bool string_builder_printf(StringBuilder *builder, const char *format, ...) {
        va_list ap;

        /* Compute reqiured size */
        va_start(ap, format);
        int n = vsnprintf(builder->str, 0, format, ap);
        va_end(ap);
        if (n < 0) {
                return false;
        }

        size_t old_len = builder->len;
        if (!string_builder_grow(builder, n)) {
                return false;
        }

        va_start(ap, format);
        vsnprintf(builder->str + old_len, n + 1, format, ap);
        va_end(ap);

        return true;
}
