/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>

#include "types.h"

int metrics_export(Manager *manager);
bool metrics_node_signal_matching_register(Node *node);
void metrics_produce_job_report(Job *job);
