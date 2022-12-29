#ifndef _BLUE_CHIHUAHUA_NODE_PEER_DBUS
#define _BLUE_CHIHUAHUA_NODE_PEER_DBUS

#include "../../common/dbus.h"

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>

char *assemble_address(const struct sockaddr_in *addr);
sd_bus *peer_dbus_new(const char *dbus_addr, sd_event *event);
bool peer_dbus_start(sd_bus *dbus);

#endif