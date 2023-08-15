/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>
#include <systemd/sd-bus.h>

int shutdown_event_loop(sd_event *event_loop);
int shutdown_service_register(sd_bus *target_bus, sd_event *event);
int service_call_shutdown(sd_bus *target_bus, const char *service_name);
int event_loop_add_shutdown_signals(sd_event *event);
