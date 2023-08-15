/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/parse-util.h"


#ifndef __FUNCTION_NAME__
#        define __FUNCTION_NAME__ __func__
#endif

bool test_parse_long(const char *input, bool expectedReturn, long expectedResult) {
        _cleanup_free_ char *msg = NULL;

        long res = 0;
        int ret = parse_long(input, &res);
        if ((ret != expectedReturn) || (res != expectedResult)) {
                ret = asprintf(&msg,
                               "FAILED: %s\n\tInput: '%s'\n\tExpected: Return=%d, Result=%ld\n\tGot: Return=%d, Result=%ld\n",
                               __FUNCTION_NAME__,
                               input,
                               expectedReturn,
                               expectedResult,
                               ret,
                               res);
        }

        if (msg != NULL) {
                fprintf(stderr, "%s", msg);
                return false;
        }
        return true;
}

bool test_parse_port(const char *input, bool expected_return, uint16_t expected_result) {
        _cleanup_free_ char *msg = NULL;

        uint16_t res = 0;
        bool ret = parse_port(input, &res);
        if ((ret != expected_return) || (res != expected_result)) {
                ret = asprintf(&msg,
                               "FAILED: %s\n\tInput: '%s'\n\tExpected: Return=%d, Result=%d\n\tGot: Return=%d, Result=%d\n",
                               __FUNCTION_NAME__,
                               input,
                               expected_return,
                               expected_result,
                               ret,
                               res);
        }

        if (msg != NULL) {
                fprintf(stderr, "%s", msg);
                return false;
        }
        return true;
}

int main() {
        bool result = true;

        result = result && test_parse_long(NULL, false, 0L);
        result = result && test_parse_long("", false, 0L);
        result = result && test_parse_long("foo", false, 0L);
        result = result && test_parse_long("1234", true, 1234L);
        result = result && test_parse_long("1234abc", false, 0L);
        result = result && test_parse_long("abc1234", false, 0L);
        result = result && test_parse_long("-1234", true, -1234L);
        result = result && test_parse_long("12.34", false, 0L);

        result = result && test_parse_port(NULL, false, 0);
        result = result && test_parse_port("", false, 0);
        result = result && test_parse_port("foo", false, 0);
        result = result && test_parse_port("0", true, 0);
        result = result && test_parse_port("8888", true, 8888);
        result = result && test_parse_port("65535", true, 65535);
        result = result && test_parse_port("65536", false, 0);
        result = result && test_parse_port("-2", false, 0);
        result = result && test_parse_port("65.536", false, 0);


        if (!result) {
                return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
}
