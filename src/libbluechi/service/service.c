/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include "libbluechi/common/common.h"
#include "libbluechi/log/log.h"

#include "service.h"

char *assemble_interface_name(char *name_postfix) {
        char *interface_name = NULL;
        int r = asprintf(&interface_name, "%s.%s", BC_INTERFACE_BASE_NAME, name_postfix);
        if (r < 0) {
                bc_log_error("Out of memory");
                return NULL;
        }
        return interface_name;
}
