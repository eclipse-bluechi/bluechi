#pragma once

#include <systemd/sd-bus.h>

sd_bus *system_bus_open(sd_event *event);
