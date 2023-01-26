#pragma once

#include <netinet/in.h>
#include <systemd/sd-bus.h>

#include "libhirte/common/list.h"

char *assemble_tcp_address(const struct sockaddr_in *addr);

sd_bus *peer_bus_open(sd_event *event, char *dbus_description, char *dbus_server_addr);
sd_bus *peer_bus_open_server(sd_event *event, char *dbus_description, int fd);
