/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <systemd/sd-bus.h>

#include "libhirte/bus/utils.h"
#include "libhirte/common/common.h"

#include "types.h"

typedef void (*print_unit_list_fn)(UnitList *unit_list);

int method_list_units_on_all(sd_bus *api_bus, print_unit_list_fn printer);
int method_list_units_on(sd_bus *api_bus, const char *name, print_unit_list_fn printer);

struct UnitList {
        int ref_count;

        LIST_FIELDS(UnitList, node_units);

        LIST_HEAD(UnitInfo, units);
};

UnitList *new_unit_list();
UnitList *unit_list_ref(UnitList *unit_list);
void unit_list_unref(UnitList *unit_list);

DEFINE_CLEANUP_FUNC(UnitList, unit_list_unref)
#define _cleanup_unit_list_ _cleanup_(unit_list_unrefp)

/* Prints 100-characters-wide table */
void print_unit_list_simple(UnitList *unit_list);