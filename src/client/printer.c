/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <stdio.h>

#include "libhirte/bus/utils.h"
#include "libhirte/common/common.h"

#include "client.h"
#include "printer.h"
#include "types.h"

/* Prints 100-characters-wide table */
void print_unit_list_simple(UnitList *unit_list) {
        UnitInfo *unit = NULL;
        printf("%-80.80s|%9s|%9s\n", "ID", "ACTIVE", "SUB");
        printf("====================================================================================================\n");
        LIST_FOREACH(units, unit, unit_list->units) {
                printf("%-80.80s|%9s|%9s\n", unit->id, unit->active_state, unit->sub_state);
        }
}

/* Prints 100-characters-wide table */
void print_nodes_unit_list_simple(UnitList *unit_list) {
        UnitInfo *unit = NULL;
        printf("%-20.20s|%-59.59s|%9s|%9s\n", "NODE", "ID", "ACTIVE", "SUB");
        printf("====================================================================================================\n");
        LIST_FOREACH(units, unit, unit_list->units) {
                printf("%-20.20s|%-59.59s|%9s|%9s\n", unit->node, unit->id, unit->active_state, unit->sub_state);
        }
}

/* We can implement here other types of printers, e.g. json printer */
Printer get_simple_printer() {
        Printer p = { .print_unit_list = &print_unit_list_simple,
                      .print_nodes_unit_list = &print_nodes_unit_list_simple };
        return p;
}

void print_unit_list(Printer *p, UnitList *unit_list) {
        (*p->print_unit_list)(unit_list);
}

void print_nodes_unit_list(Printer *p, UnitList *unit_list) {
        (*p->print_nodes_unit_list)(unit_list);
}