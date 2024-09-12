/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <systemd/sd-bus.h>

#include "libbluechi/cli/command.h"

int method_monitor(Command *command, void *userdata);
void usage_method_monitor();
