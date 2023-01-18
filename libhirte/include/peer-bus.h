#pragma once

#include <netinet/in.h>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "common/list.h"

char *assemble_tcp_address(const struct sockaddr_in *addr);

sd_bus *peer_bus_open(sd_event *event, char *dbus_description, char *dbus_server_addr);
sd_bus *peer_bus_open_server(sd_event *event, char *dbus_description, int fd);

typedef struct PeerBusListItem {
        sd_bus *bus;
        LIST_FIELDS(struct PeerBusListItem, peers);
} PeerBusListItem;

// peer_bus_list_item_new creates a new list item for a peer bus server.
// It steals the pointer from the passed in and takes care of it being cleaned up.
PeerBusListItem *peer_bus_list_item_new(sd_bus *bus);