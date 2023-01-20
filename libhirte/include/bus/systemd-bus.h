#pragma once

#include <systemd/sd-bus.h>

sd_bus *systemd_bus_open(sd_event *event);