#pragma once

#include <stdint.h>
#include <systemd/sd-bus.h>

#include "libhirte/common/common.h"

int bus_parse_property_string(sd_bus_message *m, const char *name, const char **value);

typedef struct UnitInfo UnitInfo;
struct UnitInfo {
        int ref_count;

        const char *node;
        const char *id;
        const char *description;
        const char *load_state;
        const char *active_state;
        const char *sub_state;
        const char *following;
        const char *unit_path;
        uint32_t job_id;
        const char *job_type;
        const char *job_path;

        LIST_FIELDS(UnitInfo, units);
};

UnitInfo *new_unit();
UnitInfo *unit_ref(UnitInfo *unit);
void unit_unref(UnitInfo *unit);

int bus_parse_unit_info(sd_bus_message *message, UnitInfo *u);

int assemble_object_path_string(const char *prefix, const char *name, char **res);

DEFINE_CLEANUP_FUNC(UnitInfo, unit_unref)
#define _cleanup_unit_ _cleanup_(unit_unrefp)
