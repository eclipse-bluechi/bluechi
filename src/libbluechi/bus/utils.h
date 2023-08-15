/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdint.h>
#include <systemd/sd-bus.h>

#include "libbluechi/common/common.h"

/* return < 0 for error, 0 to continue, 1 to stop, 2 to continue and skip (if value was not consumed) */
typedef int (*bus_property_cb)(const char *key, const char *value_type, sd_bus_message *m, void *userdata);

int bus_parse_properties_foreach(sd_bus_message *m, bus_property_cb cb, void *userdata);

int bus_parse_property_string(sd_bus_message *m, const char *name, const char **value);
char *bus_path_escape(const char *str);

typedef struct UnitInfo UnitInfo;
struct UnitInfo {
        int ref_count;

        char *node;
        char *id;
        char *description;
        char *load_state;
        char *active_state;
        char *sub_state;
        char *following;
        char *unit_path;
        uint32_t job_id;
        char *job_type;
        char *job_path;

        LIST_FIELDS(UnitInfo, units);
};

UnitInfo *new_unit();
UnitInfo *unit_ref(UnitInfo *unit);
void unit_unref(UnitInfo *unit);

int bus_parse_unit_info(sd_bus_message *message, UnitInfo *u);
int bus_parse_unit_on_node_info(sd_bus_message *message, UnitInfo *u);

int bus_socket_set_no_delay(sd_bus *bus);
int bus_socket_set_keepalive(sd_bus *bus);

int assemble_object_path_string(const char *prefix, const char *name, char **res);

DEFINE_CLEANUP_FUNC(UnitInfo, unit_unref)
#define _cleanup_unit_ _cleanup_(unit_unrefp)
