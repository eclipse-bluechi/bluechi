#pragma once

#include <netinet/in.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "../common/memory.h"

typedef struct {
        char *peer_dbus_addr;
        sd_bus *internal_dbus;
} PeerDBus;

char *assemble_address(const struct sockaddr_in *addr);

PeerDBus *peer_dbus_new(const char *peer_addr, sd_event *event);
void peer_dbus_unrefp(PeerDBus **dbus);

bool peer_dbus_start(PeerDBus *dbus);

#define _cleanup_peer_dbus_ _cleanup_(peer_dbus_unrefp)
