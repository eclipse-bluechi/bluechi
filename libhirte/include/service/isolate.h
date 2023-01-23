#pragma once

#include <stdbool.h>
#include <systemd/sd-bus.h>

bool service_register_isolate_all(sd_bus *target_bus, Orchestrator *orchestrator);
