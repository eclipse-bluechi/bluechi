#include <errno.h>
#include <stdio.h>
#include <systemd/sd-bus-vtable.h>

#include "libhirte/common/common.h"
#include "libhirte/common/dbus.h"
#include "libhirte/common/memory.h"
#include "service.h"
#include "shutdown.h"

int shutdown_event_loop(sd_event *event) {
        if (event == NULL) {
                return -EINVAL;
        }

        return sd_event_exit(event, 0);
}

static int method_shutdown(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *ret_error) {
        if (userdata == NULL) {
                return -EINVAL;
        }

        sd_event *event = (sd_event *) userdata;

        int r = shutdown_event_loop(event);
        if (r < 0) {
                return sd_bus_reply_method_errnof(m, -r, "Failed to shutown event loop: %m\n");
        }
        return sd_bus_reply_method_return(m, "");
}

#define method_name_shutdown "Shutdown"

// NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
static const sd_bus_vtable vtable_shutdown[] = {
        SD_BUS_VTABLE_START(0), SD_BUS_METHOD(method_name_shutdown, "", "", method_shutdown, 0), SD_BUS_VTABLE_END
};

int shutdown_service_register(sd_bus *target_bus, sd_event *event) {
        if (target_bus == NULL || event == NULL) {
                return -EINVAL;
        }

        _cleanup_free_ char *interface_name = assemble_interface_name(method_name_shutdown);
        return sd_bus_add_object_vtable(
                        target_bus, NULL, HIRTE_OBJECT_PATH, interface_name, vtable_shutdown, event);
}


int service_call_shutdown(sd_bus *target_bus, const char *service_name) {
        if (target_bus == NULL || service_name == NULL) {
                return -EINVAL;
        }

        _cleanup_free_ char *interface_name = assemble_interface_name(method_name_shutdown);
        if (interface_name == NULL) {
                return -1;
        }

        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *reply = NULL;

        int r = sd_bus_call_method(
                        target_bus,
                        service_name,
                        HIRTE_OBJECT_PATH,
                        interface_name,
                        method_name_shutdown,
                        &error,
                        &reply,
                        "");
        if (r < 0) {
                fprintf(stderr, "Failed to call '%s': %s\n", method_name_shutdown, error.message);
                return r;
        }
        return 0;
}

static int event_loop_signal_handler(
                sd_event_source *event_source, UNUSED const struct signalfd_siginfo *si, UNUSED void *userdata) {
        if (event_source == NULL) {
                return -EINVAL;
        }

        sd_event *event = sd_event_source_get_event(event_source);
        return shutdown_event_loop(event);
}

int event_loop_add_shutdown_signals(sd_event *event) {
        sigset_t sigset;
        int r = 0;

        if (event == NULL) {
                return -EINVAL;
        }

        // Block this thread from handling SIGTERM and SIGINT so that these
        // signals can be handled by the event loop instead.
        r = sigemptyset(&sigset);
        if (r < 0) {
                fprintf(stderr, "sigemptyset() failed: %m\n");
                return -1;
        }
        r = sigaddset(&sigset, SIGTERM);
        if (r < 0) {
                fprintf(stderr, "sigaddset() failed: %m\n");
                return -1;
        }
        r = sigaddset(&sigset, SIGINT);
        if (r < 0) {
                fprintf(stderr, "sigaddset() failed: %m\n");
                return -1;
        }
        r = sigprocmask(SIG_BLOCK, &sigset, NULL);
        if (r < 0) {
                fprintf(stderr, "sigprocmask() failed: %m\n");
                return -1;
        }

        // Add SIGTERM and SIGINT as event sources in the event loop.
        r = sd_event_add_signal(event, NULL, SIGTERM, event_loop_signal_handler, NULL);
        if (r < 0) {
                fprintf(stderr, "sd_event_add_signal() failed: %m\n");
                return -1;
        }
        r = sd_event_add_signal(event, NULL, SIGINT, event_loop_signal_handler, NULL);
        if (r < 0) {
                fprintf(stderr, "sd_event_add_signal() failed: %m\n");
                return -1;
        }

        return 0;
}
