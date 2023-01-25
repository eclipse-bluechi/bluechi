#include <stdio.h>

#include "../../include/common/common.h"
#include "../../include/job-manager.h"
#include "../../include/service/node.h"
#include "../../include/service/service.h"
#include "../common/memory.h"

static int method_node_start_unit(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *target = NULL;
        int r = sd_bus_message_read(m, "s", &target);
        if (r < 0) {
                return sd_bus_reply_method_errnof(m, -r, "Failed to create job for starting systemd unit: %m");
        }
        fprintf(stdout, "Starting systemd unit '%s'...\n", target);

        // TODO: queue start job

        return sd_bus_reply_method_return(m, "o", "/this/is/currently/invalid");
}

static int method_node_stop_unit(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *target = NULL;
        int r = sd_bus_message_read(m, "s", &target);
        if (r < 0) {
                return sd_bus_reply_method_errnof(m, -r, "Failed to create job for stopping systemd unit: %m");
        }
        fprintf(stdout, "Stopping systemd unit '%s'...\n", target);

        // TODO: queue stop job

        return sd_bus_reply_method_return(m, "b", true);
}

static int method_node_restart_unit(sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        const char *target = NULL;
        int r = sd_bus_message_read(m, "s", &target);
        if (r < 0) {
                return sd_bus_reply_method_errnof(
                                m, -r, "Failed to create job for restarting systemd unit: %m");
        }
        fprintf(stdout, "Restarting systemd unit '%s'...\n", target);

        // TODO: queue restart job

        return sd_bus_reply_method_return(m, "o", "/this/is/currently/invalid");
}

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
static const sd_bus_vtable vtable_node_systemd[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("StartUnit", "s", "o", method_node_start_unit, 0),
        SD_BUS_METHOD("StopUnit", "s", "b", method_node_stop_unit, 0),
        SD_BUS_METHOD("RestartUnit", "s", "o", method_node_restart_unit, 0),
        SD_BUS_VTABLE_END
};

int node_systemd_service_register(sd_bus *target_bus) {
        _cleanup_free_ char *interface_name = assemble_interface_name("Systemd");

        JobManager *mgr = job_manager_new();

        return sd_bus_add_object_vtable(
                        target_bus, NULL, HIRTE_OBJECT_PATH, interface_name, vtable_node_systemd, mgr);
}
