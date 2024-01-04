/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>
#include <systemd/sd-bus.h>

int shutdown_event_loop(sd_event *event_loop);
int event_loop_add_shutdown_signals(sd_event *event);
