#ifndef _BLUE_CHIHUAHUA_NODE_PEER_DBUS
#define _BLUE_CHIHUAHUA_NODE_PEER_DBUS

#include "../common/dbus.h"

#include <netinet/in.h>

int setup_peer_dbus(sd_bus *dbus, const struct sockaddr_in *addr);

#endif