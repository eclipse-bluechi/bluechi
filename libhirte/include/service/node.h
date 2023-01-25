#pragma once

#include <systemd/sd-bus.h>

int node_systemd_service_register(sd_bus *target_bus);