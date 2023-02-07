#include "monitor.h"
#include "job.h"
#include "manager.h"
#include "node.h"

#define DEBUG_AGENT_MESSAGES 0

static int monitor_method_close(sd_bus_message *m, void *userdata, sd_bus_error *ret_error);

static const sd_bus_vtable monitor_vtable[] = { SD_BUS_VTABLE_START(0),
                                                SD_BUS_METHOD("Close", "", "", monitor_method_close, 0),
                                                SD_BUS_VTABLE_END };

Monitor *monitor_new(Manager *manager, const char *client) {
        static uint32_t next_id = 0;
        _cleanup_monitor_ Monitor *monitor = malloc0(sizeof(Monitor));
        if (monitor == NULL) {
                return NULL;
        }

        monitor->ref_count = 1;
        monitor->manager = manager;
        LIST_INIT(monitors, monitor);
        monitor->id = ++next_id;

        monitor->client = strdup(client);
        if (monitor->client == NULL) {
                return NULL;
        }

        int r = asprintf(&monitor->object_path, "%s/%u", MONITOR_OBJECT_PATH_PREFIX, monitor->id);
        if (r < 0) {
                return NULL;
        }

        return steal_pointer(&monitor);
}

Monitor *monitor_ref(Monitor *monitor) {
        monitor->ref_count++;
        return monitor;
}

void monitor_unref(Monitor *monitor) {
        monitor->ref_count--;
        if (monitor->ref_count != 0) {
                return;
        }

        sd_bus_slot_unrefp(&monitor->export_slot);
        free(monitor->object_path);
        free(monitor->client);
        free(monitor);
}

bool monitor_export(Monitor *monitor) {
        Manager *manager = monitor->manager;

        int r = sd_bus_add_object_vtable(
                        manager->user_dbus,
                        &monitor->export_slot,
                        monitor->object_path,
                        MONITOR_INTERFACE,
                        monitor_vtable,
                        monitor);
        if (r < 0) {
                fprintf(stderr, "Failed to add monitor vtable: %s\n", strerror(-r));
                return false;
        }

        return true;
}

void monitor_close(Monitor *monitor) {
        sd_bus_slot_unrefp(&monitor->export_slot);
        monitor->export_slot = NULL;
}

static int monitor_method_close(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        _cleanup_monitor_ Monitor *monitor = monitor_ref(userdata);
        Manager *manager = monitor->manager;

        /* Ensure we don't close it twice somehow */
        if (monitor->export_slot == NULL) {
                return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_FAILED, "Internal error");
        }

        monitor_close(monitor);
        manager_remove_monitor(manager, monitor);

        return sd_bus_reply_method_return(m, "");
}
