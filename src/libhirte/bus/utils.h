#pragma once

#include <systemd/sd-bus.h>

int bus_parse_property_string(sd_bus_message *m, const char *name, const char **value);
