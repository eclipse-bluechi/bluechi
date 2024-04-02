/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <stdbool.h>
#include <systemd/sd-bus.h>

typedef void (*ShutdownHookFn)(void *userdata);

typedef struct ShutdownHook {
        ShutdownHookFn shutdown;
        void *userdata;
} ShutdownHook;

int shutdown_event_loop(sd_event *event_loop);
int event_loop_add_shutdown_signals(sd_event *event, ShutdownHook *hook);
