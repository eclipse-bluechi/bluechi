/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/cli/command.h"

int method_start(Command *command, void *userdata);
int method_stop(Command *command, void *userdata);
int method_restart(Command *command, void *userdata);
int method_reload(Command *command, void *userdata);
void usage_method_lifecycle();
