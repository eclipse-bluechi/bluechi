#include "../../src/common/memory.h"
#include "../../src/common/parse-util.h"

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

#ifndef __FUNCTION_NAME__
#        define __FUNCTION_NAME__ __func__
#endif

void test_parse_long(const char *input, bool expectedReturn, long expectedResult) {
        char *msg = NULL;

        long res = 0;
        int ret = parse_long(input, &res);
        if ((ret != expectedReturn) || (res != expectedResult)) {
                asprintf(&msg,
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
        }
}

void test_parse_port(const char *input, bool expected_return, uint16_t expected_result) {
        char *msg = NULL;

        uint16_t res = 0;
        bool ret = parse_port(input, &res);
        if ((ret != expected_return) || (res != expected_result)) {
                asprintf(&msg,
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
        }
}

int main() {
        test_parse_long(NULL, false, 0L);
        test_parse_long("", false, 0L);
        test_parse_long("foo", false, 0L);
        test_parse_long("1234", true, 1234L);
        test_parse_long("1234abc", false, 1234L);
        test_parse_long("abc1234", false, 0L);

        test_parse_port(NULL, false, 0);
        test_parse_port("", false, 0);
        test_parse_port("foo", false, 0);
        test_parse_port("0", true, 0);
        test_parse_port("8888", true, 8888);
        test_parse_port("65535", true, 65535);
        test_parse_port("65536", false, 0);
        test_parse_port("-2", false, 0);
}
