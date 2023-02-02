#pragma once

#include "types.h"

struct Printer {
        void (*print_unit_list)(UnitList *);
        void (*print_nodes_unit_list)(NodesUnitList *);
};

Printer get_simple_printer();

void print_unit_list(Printer *p, UnitList *unit_list);
void print_nodes_unit_list(Printer *p, NodesUnitList *nodes_unit_list);