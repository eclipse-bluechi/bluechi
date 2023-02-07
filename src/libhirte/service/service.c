#include "service.h"
#include "libhirte/common/common.h"
#include "libhirte/log/log.h"

char *assemble_interface_name(char *name_postfix) {
        char *interface_name = NULL;
        int r = asprintf(&interface_name, "%s.%s", HIRTE_INTERFACE_BASE_NAME, name_postfix);
        if (r < 0) {
                hirte_log_error("Out of memory\n");
                return NULL;
        }
        return interface_name;
}
