/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/cli/command.h"

int method_enable(Command *command, void *userdata);
void usage_method_enable();

int method_disable(Command *command, void *userdata);
void usage_method_disable();
