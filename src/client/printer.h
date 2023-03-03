/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "types.h"

struct Printer {
        void (*print_unit_list)(UnitList *);
        void (*print_nodes_unit_list)(UnitList *);
};

Printer get_simple_printer();

void print_unit_list(Printer *p, UnitList *unit_list);
void print_nodes_unit_list(Printer *p, UnitList *nodes_unit_list);