/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/string-util.h"

static bool check_builder_len(const char *test, StringBuilder *builder, size_t expected_len) {
        if (builder->len != expected_len) {
                fprintf(stdout,
                        "FAILED: %s - Expected Length: '%zd', but got '%zd'\n",
                        test,
                        expected_len,
                        builder->len);
                return false;
        }
        return true;
}


static bool check_builder(const char *test, StringBuilder *builder, const char *expected_str) {
        if (!streq(builder->str, expected_str)) {
                fprintf(stdout, "FAILED: %s - Expected: '%s', but got '%s'\n", test, expected_str, builder->str);
                return false;
        }
        return check_builder_len(test, builder, strlen(expected_str));
}

bool test_append(void) {
        _cleanup_string_builder_ StringBuilder builder = STRING_BUILDER_INIT;

        /* Start with low capacity to verify growing */
        if (!string_builder_init(&builder, 1)) {
                fprintf(stdout, "FAILED: string_builder_init() - Unexpected failure\n");
                return false;
        }

        if (!string_builder_append(&builder, "string1")) {
                fprintf(stdout, "FAILED: string_builder_append() - Unexpected failure\n");
                return false;
        }

        if (!check_builder("test_append 1", &builder, "string1")) {
                return false;
        }

        if (!string_builder_append(&builder, "string2")) {
                fprintf(stdout, "FAILED: string_builder_append() - Unexpected failure\n");
                return false;
        }

        if (!check_builder("test_append 2", &builder, "string1string2")) {
                return false;
        }

        for (int i = 0; i < 1000; i++) {
                if (!string_builder_append(&builder, "XXXXXXXXXX")) {
                        fprintf(stdout, "FAILED: string_builder_append() - Unexpected failure\n");
                        return false;
                }
        }

        if (!check_builder_len(
                            "test_append 3", &builder, strlen("string1string2") + 1000 * strlen("XXXXXXXXXX"))) {
                return false;
        }

        return true;
}

bool test_printf(void) {
        _cleanup_string_builder_ StringBuilder builder = STRING_BUILDER_INIT;

        /* Start with low capacity to verify growing */
        if (!string_builder_init(&builder, 1)) {
                fprintf(stdout, "FAILED: string_builder_init() - Unexpected failure\n");
                return false;
        }

        if (!string_builder_printf(&builder, "string%d", 1)) {
                fprintf(stdout, "FAILED: string_builder_printf() - Unexpected failure\n");
                return false;
        }

        if (!check_builder("test_printf 1", &builder, "string1")) {
                return false;
        }

        if (!string_builder_printf(&builder, "string%s", "2")) {
                fprintf(stdout, "FAILED: string_builder_printf() - Unexpected failure\n");
                return false;
        }

        if (!check_builder("test_printf 2", &builder, "string1string2")) {
                return false;
        }

        for (int i = 0; i < 1000; i++) {
                if (!string_builder_printf(&builder, "%10s", "X")) {
                        fprintf(stdout, "FAILED: string_builder_print() - Unexpected failure\n");
                        return false;
                }
        }

        if (!check_builder_len(
                            "test_printf 3", &builder, strlen("string1string2") + 1000 * strlen("XXXXXXXXXX"))) {
                return false;
        }

        return true;
}


int main() {
        bool result = true;

        result = result && test_append();
        result = result && test_printf();

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
