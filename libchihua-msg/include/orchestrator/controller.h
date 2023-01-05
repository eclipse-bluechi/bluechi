#ifndef _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR_CONTROLLER
#define _BLUE_CHIHUAHUA_LIB_ORCHESTRATOR_CONTROLLER

#include "../common/memory.h"

#include <stdbool.h>
#include <systemd/sd-event.h>

// TODO: include common/dbus and cleanup event_source in _new

typedef struct {
        int accept_fd;
        sd_event_source *sd_event_source;
} Controller;

Controller *controller_new(uint16_t port, sd_event *event, sd_event_io_handler_t accept_handler);
void controller_unrefp(Controller **controller);

sd_event_io_handler_t default_accept_handler();

#define _cleanup_controller_ _cleanup_(controller_unrefp)

#endif