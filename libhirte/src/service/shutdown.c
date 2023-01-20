#include <stdio.h>
#include <systemd/sd-bus-vtable.h>

#include "../../include/common/common.h"
#include "../../include/service/shutdown.h"
#include "../common/memory.h"
#include "service.h"

int shutdown_event_loop(sd_event *event_loop) {
        if (event_loop == NULL) {
                return 0;
        }

        return sd_event_exit(event_loop, 0);
}

static int method_shutdown(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        sd_event *event_loop = (sd_event *) userdata;

        int r = shutdown_event_loop(event_loop);
        if (r < 0) {
                return sd_bus_reply_method_errnof(m, -r, "Failed to shutown event loop: %m\n");
        }
        return sd_bus_reply_method_return(m, "");
}

static const sd_bus_vtable my_vtable[] = { SD_BUS_VTABLE_START(0),
                                           SD_BUS_METHOD("Shutdown", "", "", method_shutdown, 0),
                                           SD_BUS_VTABLE_END };

bool service_register_shutdown(sd_bus *target_bus, sd_event *event_loop) {
        _cleanup_free_ char *interface_name = assemble_interface_name("Shutdown");
        int r = sd_bus_add_object_vtable(
                        target_bus, NULL, HIRTE_OBJECT_PATH, interface_name, my_vtable, event_loop);
        if (r < 0) {
                fprintf(stderr, "Failed to register shutdown service: %s\n", strerror(-r));
                return false;
        }
        return true;
}