#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include "types.h"

#include "libhirte/common/common.h"

struct Manager {
        int ref_count;

        uint16_t port;
        char *user_bus_service_name;

        sd_event *event;
        sd_event_source *node_connection_source;

        sd_bus *user_dbus;

        LIST_HEAD(ManagedNode, nodes);
        LIST_HEAD(ManagedNode, anonymous_nodes);
};

Manager *manager_new(void);
Manager *manager_ref(Manager *manager);
void manager_unref(Manager *manager);
void manager_unrefp(Manager **managerp);

bool manager_set_port(Manager *manager, const char *port);
bool manager_parse_config(Manager *manager, const char *configfile);

bool manager_start(Manager *manager);
bool manager_stop(Manager *manager);

ManagedNode *manager_find_node(Manager *manager, const char *name);
void manager_remove_node(Manager *manager, ManagedNode *node);

ManagedNode *manager_add_node(Manager *manager, const char *name);

#define _cleanup_manager_ _cleanup_(manager_unrefp)
