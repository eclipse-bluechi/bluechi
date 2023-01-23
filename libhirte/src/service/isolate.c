#include <errno.h>
#include <stdio.h>
#include <systemd/sd-bus-vtable.h>

#include "../../include/orchestrator.h"
#include "../../include/common/common.h"
#include "../../include/service/service.h"
#include "../../include/service/isolate.h"
#include "../common/memory.h"

bool service_register(sd_bus *target_bus, char *interface_name_postfix, const sd_bus_vtable *vtable, void *user_data) {
        _cleanup_free_ char *interface_name = assemble_interface_name(interface_name_postfix);
        int r = sd_bus_add_object_vtable(
                        target_bus, NULL, HIRTE_OBJECT_PATH, interface_name, vtable, user_data);
        if (r < 0) {
                fprintf(stderr, "Failed to register %s service: %s\n", interface_name_postfix, strerror(-r));
                return false;
        }
        printf("Service on interface %s was added\n", interface_name);
        return true;
}

static int method_orchestrator_isolate_all(UNUSED sd_bus_message *m, UNUSED void *userdata, UNUSED sd_bus_error *ret_error) {
        printf("Running IsolateAll method");
        return 0;
}

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
static const sd_bus_vtable vtable_isolate_all[] = {
        SD_BUS_VTABLE_START(0),
        SD_BUS_METHOD("IsolateAll", "s", "o", method_orchestrator_isolate_all, 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobNew",
                                 "uo",
                                 SD_BUS_PARAM(id)
                                 SD_BUS_PARAM(job),
                                 0),
        SD_BUS_SIGNAL_WITH_NAMES("JobRemoved",
                                 "uos",
                                 SD_BUS_PARAM(id)
                                 SD_BUS_PARAM(job)
                                 SD_BUS_PARAM(result),
                                 0),
        SD_BUS_VTABLE_END
};

bool service_register_isolate_all(sd_bus *target_bus, Orchestrator *orchestrator) {
        return service_register(target_bus, "IsolateAll", vtable_isolate_all, orchestrator);
}