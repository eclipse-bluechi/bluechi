#pragma once

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "memory.h"

#define _cleanup_sd_event_ _cleanup_(sd_event_unrefp)
#define _cleanup_sd_event_source_ _cleanup_(sd_event_source_unrefp)
#define _cleanup_sd_bus_ _cleanup_(sd_bus_unrefp)
#define _cleanup_sd_bus_slot_ _cleanup_(sd_bus_slot_unrefp)
#define _cleanup_sd_bus_message_ _cleanup_(sd_bus_message_unrefp)
