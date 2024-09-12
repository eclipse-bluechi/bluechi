/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/cli/command.h"

int method_freeze(Command *command, void *userdata);
void usage_method_freeze();

int method_thaw(Command *command, void *userdata);
void usage_method_thaw();
