#pragma once

#include "libhirte/common/common.h"

#include "types.h"

struct Monitor {
        int ref_count;
        uint32_t id;
        char *client;

        Manager *manager; /* weak ref */

        sd_bus_slot *export_slot;
        char *object_path;

        LIST_FIELDS(Monitor, monitors);
};

Monitor *monitor_new(Manager *manager, const char *client);
Monitor *monitor_ref(Monitor *monitor);
void monitor_unref(Monitor *monitor);

void monitor_close(Monitor *monitor);

bool monitor_export(Monitor *monitor);

DEFINE_CLEANUP_FUNC(Monitor, monitor_unref)
#define _cleanup_monitor_ _cleanup_(monitor_unrefp)
