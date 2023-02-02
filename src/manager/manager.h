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
        sd_bus_slot *manager_slot;

        int n_nodes;
        LIST_HEAD(Node, nodes);
        LIST_HEAD(Node, anonymous_nodes);

        LIST_HEAD(Job, jobs);
};

Manager *manager_new(void);
Manager *manager_ref(Manager *manager);
void manager_unref(Manager *manager);
void manager_unrefp(Manager **managerp);

bool manager_set_port(Manager *manager, const char *port);
bool manager_parse_config(Manager *manager, const char *configfile);

bool manager_start(Manager *manager);
bool manager_stop(Manager *manager);

Node *manager_find_node(Manager *manager, const char *name);
void manager_remove_node(Manager *manager, Node *node);

Node *manager_add_node(Manager *manager, const char *name);

bool manager_add_job(Manager *manager, Job *job);
void manager_remove_job(Manager *manager, Job *job, const char *result);
void manager_finish_job(Manager *manager, uint32_t job_id, const char *result);

#define _cleanup_manager_ _cleanup_(manager_unrefp)
