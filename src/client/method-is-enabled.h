/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <stddef.h>

#include "libbluechi/cli/command.h"

int method_is_enabled(Command *command, void *userdata);
void usage_method_is_enabled();
