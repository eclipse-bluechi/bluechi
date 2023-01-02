#ifndef _BLUE_CHIHUAHUA_ORCH_PARSEUTIL
#define _BLUE_CHIHUAHUA_ORCH_PARSEUTIL

#include <inttypes.h>
#include <stdbool.h>

bool parse_long(const char *in, long *ret);
bool parse_port(const char *in, uint16_t *ret);

#endif
