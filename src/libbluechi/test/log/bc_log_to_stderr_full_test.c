/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "libbluechi/log/log.h"

LogLevel lvl = LOG_LEVEL_INFO;
const char *file = "testfile.c";
const char *line = "line10";
const char *func = "testfunc";
const char *msg = "log_message";

bool check_buffer(char buf[]) {
        bool result = true;
        if (!strstr(buf, log_level_to_string(lvl))) {
                result = false;
                fprintf(stderr, "FAILED: LogLevel not found in log entry");
        }
        if (!strstr(buf, file)) {
                result = false;
                fprintf(stderr, "FAILED: File location not found in log entry");
        }
        if (!strstr(buf, line)) {
                result = false;
                fprintf(stderr, "FAILED: Line location not found in log entry");
        }
        if (!strstr(buf, func)) {
                result = false;
                fprintf(stderr, "FAILED: Function name not found in log entry");
        }
        if (!strstr(buf, msg)) {
                result = false;
                fprintf(stderr, "FAILED: Message not found in log entry");
        }

        return result;
}

int main() {
        bool result = true;

        char buf[BUFSIZ] = { '\0' };

        /* test case: log without data */

        // set buffer to stderr
        setbuf(stderr, buf);
        bc_log_to_stderr_full_with_location(lvl, file, line, func, msg, NULL);
        // avoid adding test logs to buffer
        setbuf(stderr, NULL);
        result = check_buffer(buf);
        if (strstr(buf, "data=")) {
                result = false;
                fprintf(stderr, "FAILED: Data found in log entry, but none was expected");
        }

        /* test case: log with data */

        // set buffer to stderr
        setbuf(stderr, buf);
        const char *data = "{data: this is it}";
        bc_log_to_stderr_full_with_location(lvl, file, line, func, msg, data);
        // avoid adding test logs to buffer
        setbuf(stderr, NULL);
        result = check_buffer(buf);
        if (!strstr(buf, data)) {
                result = false;
                fprintf(stderr, "FAILED: Data not found in log entry, but was expected");
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
