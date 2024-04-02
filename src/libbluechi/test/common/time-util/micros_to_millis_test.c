/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <assert.h>
#include <stdlib.h>

#include "libbluechi/common/time-util.h"


int main() {
        assert(micros_to_millis(0) == 0);
        assert(micros_to_millis(12) == 0.012);
        assert(micros_to_millis(UINT64_MAX) == 18446744073709552);
        return EXIT_SUCCESS;
}
