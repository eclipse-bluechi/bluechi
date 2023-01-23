#pragma once

#include <stdbool.h>
#include <systemd/sd-bus.h>

int shutdown_event_loop(sd_event *event_loop);
bool shutdown_service_register(sd_bus *target_bus, sd_event *event);
bool service_call_shutdown(sd_bus *target_bus, const char *service_name);

int event_loop_add_shutdown_signals(sd_event *event);
