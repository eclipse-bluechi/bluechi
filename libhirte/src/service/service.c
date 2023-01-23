#include <errno.h>
#include <stdio.h>

#include "../../include/service/service.h"

char *assemble_interface_name(char *name_postfix) {
        char *interface_name = NULL;
        int r = asprintf(&interface_name, "%s.%s", HIRTE_INTERFACE_BASE_NAME, name_postfix);
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }
        return interface_name;
}