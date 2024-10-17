/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/cli/command.h"

int method_is_online_agent(Command *command, void *userdata);
int method_is_online_node(Command *command, void *userdata);
int method_is_online_system(Command *command, void *userdata);
