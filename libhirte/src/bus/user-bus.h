#pragma once

#include <systemd/sd-bus.h>

sd_bus *user_bus_open(sd_event *event);
