#ifndef _BLUE_CHIHUAHUA_ORCH_CONTROLLER
#define _BLUE_CHIHUAHUA_ORCH_CONTROLLER

#include "../common/dbus.h"

int controller_setup(int accept_fd, uint16_t port, sd_event *event, sd_event_source *event_source);

#endif