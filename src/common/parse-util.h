#ifndef _BLUE_CHIHUAHUA_ORCH_PARSEUTIL
#define _BLUE_CHIHUAHUA_ORCH_PARSEUTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_long(char *in, long *val) {
        char *ptr;
        *val = strtol(in, &ptr, 10);
        if (strcmp(ptr, "") != 0) {
                return 1;
        }
        return 0;
}

#endif