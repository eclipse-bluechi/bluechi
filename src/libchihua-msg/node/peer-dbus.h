#ifndef _BLUE_CHIHUAHUA_NODE_PEER_DBUS
#define _BLUE_CHIHUAHUA_NODE_PEER_DBUS

#include "../../common/dbus.h"

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>

typedef struct {
        char *peer_dbus_addr;
        sd_bus *internal_dbus;
} PeerDBus;

char *assemble_address(const struct sockaddr_in *addr);

PeerDBus *peer_dbus_new(const char *peer_addr, sd_event *event);
void peer_dbus_unrefp(PeerDBus **p);

bool peer_dbus_start(PeerDBus *dbus);

#define _cleanup_peer_dbus_ _cleanup_(peer_dbus_unrefp)

#endif