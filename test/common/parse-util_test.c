#include "../../src/common/memory.h"
#include "../../src/common/parse-util.h"

#include <assert.h>
#include <stdio.h>

#ifndef __FUNCTION_NAME__
#        define __FUNCTION_NAME__ __func__
#endif

void test_parse_long(char *input, int expectedReturn, long expectedResult) {
        char *msg = NULL;

        long res;
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

int main() {
        test_parse_long("", 0, 0L);
        test_parse_long("foo", 1, 0L);
        test_parse_long("1234", 0, 1234L);
        test_parse_long("1234abc", 1, 1234L);
        test_parse_long("abc1234", 1, 0);
}
