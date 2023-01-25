#include <errno.h>

#include "../../include/service/service.h"
#include "../../include/service/systemd.h"
#include "../common/dbus.h"

#define SYSTEMD_METHOD_SUBSCRIBE "Subscribe"

// todo: remove when shutdown pr is merged
#define _cleanup_sd_bus_error_ _cleanup_(sd_bus_error_free)

int systemd_call_subscribe(sd_bus *target_bus) {
        if (target_bus == NULL) {
                return -EINVAL;
        }

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;

        int r = sd_bus_call_method(
                        target_bus,
                        SYSTEMD_BUS_NAME,
                        SYSTEMD_OBJECT_PATH,
                        SYSTEMD_MANAGER_IFACE,
                        SYSTEMD_METHOD_SUBSCRIBE,
                        &error,
                        &reply,
                        "");

        if (r < 0) {
                fprintf(stderr, "Failed to call '%s': %s\n", SYSTEMD_METHOD_SUBSCRIBE, error.message);
                return r;
        }
        return 0;
}
