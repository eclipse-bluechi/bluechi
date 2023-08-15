/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <netinet/in.h>
#include <systemd/sd-bus.h>


sd_bus *peer_bus_open(sd_event *event, const char *dbus_description, const char *dbus_server_addr);
int peer_bus_close(sd_bus *peer_dbus);
sd_bus *peer_bus_open_server(sd_event *event, const char *dbus_description, const char *sender, int fd);
sd_bus *system_bus_open(sd_event *event);
sd_bus *systemd_bus_open(sd_event *event);
sd_bus *user_bus_open(sd_event *event);
