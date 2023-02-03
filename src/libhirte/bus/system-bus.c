#include <stdio.h>

#include "libhirte/common/common.h"
#include "system-bus.h"

sd_bus *system_bus_open(sd_event *event) {
        int r = 0;
        _cleanup_sd_bus_ sd_bus *bus = NULL;

        r = sd_bus_open_system(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to connect to system bus: %s\n", strerror(-r));
                return NULL;
        }

        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return NULL;
        }
        (void) sd_bus_set_description(bus, "system-bus");

        return steal_pointer(&bus);
}
