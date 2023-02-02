#include <stdio.h>

#include "client.h"
#include "libhirte/bus/utils.h"
#include "libhirte/common/common.h"
#include "printer.h"
#include "types.h"

void print_unit_list_simple(UnitList *unit_list) {
        UnitInfo *unit = NULL;
        LIST_FOREACH(units, unit, unit_list->units) {
                printf("%s\n", unit->id);
        }
}

void print_nodes_unit_list_simple(UNUSED NodesUnitList *nodes_unit_list) {
        printf("Printing Nodes Unit List\n");
}

Printer get_simple_printer() {
        Printer p = { .print_unit_list = &print_unit_list_simple,
                      .print_nodes_unit_list = &print_nodes_unit_list_simple };
        return p;
}

void print_unit_list(Printer *p, UnitList *unit_list) {
        (*p->print_unit_list)(unit_list);
}

void print_nodes_unit_list(Printer *p, NodesUnitList *nodes_unit_list) {
        (*p->print_nodes_unit_list)(nodes_unit_list);
}