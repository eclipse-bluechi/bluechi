#pragma once

#include <stdbool.h>
#include <systemd/sd-event.h>

#include "../common/memory.h"

typedef struct {
        int accept_fd;
        sd_event_source *sd_event_source;
} Controller;

Controller *controller_new(uint16_t port, sd_event *event, sd_event_io_handler_t accept_handler);
void controller_unrefp(Controller **controller);

sd_event_io_handler_t default_accept_handler();

#define _cleanup_controller_ _cleanup_(controller_unrefp)
