#ifndef _BLUE_CHIHUAHUA_ORCH_STRINGS
#define _BLUE_CHIHUAHUA_ORCH_STRINGS

#include <string.h>

#define streq(a, b) (strcmp((a), (b)) == 0)
#define strneq(a, b, n) (strncmp((a), (b), (n)) == 0)

#endif