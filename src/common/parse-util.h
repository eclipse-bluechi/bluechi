#ifndef _BLUE_CHIHUAHUA_ORCH_PARSEUTIL
#define _BLUE_CHIHUAHUA_ORCH_PARSEUTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_long(const char *in, long *val) {
        if (in == NULL) {
                return 1;
        }

        const int decimalBase = 10;
        char *ptr = NULL;
        *val = strtol(in, &ptr, decimalBase);
        if (strcmp(ptr, "") != 0) {
                return 1;
        }
        return 0;
}

#endif