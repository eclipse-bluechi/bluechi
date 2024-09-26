/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/cli/command.h"

int method_get_default_target(Command *command, void *userdata);
void usage_method_get_default_target();
int method_set_default_target(Command *command, void *userdata);
void usage_method_set_default_target();
