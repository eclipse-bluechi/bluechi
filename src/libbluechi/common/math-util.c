/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "math-util.h"

unsigned long umaxl(unsigned long x, unsigned long y) {
        if (x > y) {
                return x;
        }
        return y;
}
