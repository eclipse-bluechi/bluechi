/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/string-util.h"

bool test_is_glob(const char *in, bool expected) {
        bool result = is_glob(in);
        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: is_glob('%s') - Expected %s, but got %s\n",
                in,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}

bool test_match_glob(const char *in, const char *glob, bool expected) {
        bool result = match_glob(in, glob);
        if (result == expected) {
                return true;
        }
        fprintf(stdout,
                "FAILED: match_glob('%s', '%s') - Expected %s, but got %s\n",
                in,
                glob,
                bool_to_str(expected),
                bool_to_str(result));
        return false;
}

int main() {
        bool result = true;

        result = result && test_is_glob(NULL, false);
        result = result && test_is_glob("", false);
        result = result && test_is_glob(".", false);
        result = result && test_is_glob("noglob", false);
        result = result && test_is_glob("isglob?", true);
        result = result && test_is_glob("?isglob", true);
        result = result && test_is_glob("isglob*", true);
        result = result && test_is_glob("*isglob", true);
        result = result && test_is_glob("*isglob?", true);
        result = result && test_is_glob("isg?ob", true);
        result = result && test_is_glob("isg*ob", true);
        result = result && test_is_glob("glob.ch?ck.serv*", true);

        result = result && test_match_glob(NULL, NULL, false);
        result = result && test_match_glob("", NULL, false);
        result = result && test_match_glob(NULL, "", false);
        result = result && test_match_glob("", "", true);
        result = result && test_match_glob("", "?", false);
        result = result && test_match_glob("", "*", true);
        result = result && test_match_glob("x", "?", true);
        result = result && test_match_glob("x", "*", true);
        result = result && test_match_glob("xt", "?", false);
        result = result && test_match_glob("xt", "?t", true);
        result = result && test_match_glob("xt", "*t", true);
        result = result && test_match_glob("glob.check.service", "?", false);
        result = result && test_match_glob("glob.check.service", "*", true);
        result = result && test_match_glob("glob.check.service", "*.check.*", true);
        result = result && test_match_glob("glob.check.service", "*.ch??k.*", true);
        result = result && test_match_glob("glob.check.service", "*.ch?k.*", false);
        result = result && test_match_glob("glob.check.service", "*.service", true);
        result = result && test_match_glob("glob.check.service", "*.service*", true);
        result = result && test_match_glob("glob.check.service", "*.service?", false);

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
