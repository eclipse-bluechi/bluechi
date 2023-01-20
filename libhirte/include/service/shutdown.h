#pragma once

#include <stdbool.h>
#include <systemd/sd-bus.h>

int shutdown_event_loop(sd_event *event_loop);
bool service_register_shutdown(sd_bus *target_bus, sd_event *event_loop);