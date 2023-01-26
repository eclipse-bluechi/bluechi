#include <stdio.h>
#include <string.h>

#include "libhirte/bus/user-bus.h"
#include "libhirte/common/common.h"
#include "libhirte/common/dbus.h"
#include "libhirte/socket.h"
#include "peer-manager.h"
#include "orchestrator.h"

static bool orch_setup_connection_handler(
                Orchestrator *orch, uint16_t listen_port, sd_event_io_handler_t connection_callback) {
        if (orch == NULL) {
                fprintf(stderr, "Orchestrator is NULL\n");
                return false;
        }
        if (orch->event == NULL) {
                fprintf(stderr, "event loop of Orchestrator is NULL\n");
                return false;
        }
        if (orch->peer_manager == NULL) {
                fprintf(stderr, "peer manager of Orchestrator is NULL\n");
                return false;
        }

        int r = 0;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        _cleanup_fd_ int accept_fd = create_tcp_socket(listen_port);
        if (accept_fd < 0) {
                return false;
        }

        r = sd_event_add_io(
                        orch->event, &event_source, accept_fd, EPOLLIN, connection_callback, orch->peer_manager);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %s\n", strerror(-r));
                return false;
        }
        // sd_event_add_io takes care of closing accept_fd, setting it to -1 avoids closing it multiple times
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        accept_fd = -1;

        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %s\n", strerror(-r));
                return false;
        }
        (void) sd_event_source_set_description(event_source, "master-socket");

        orch->peer_connection_source = steal_pointer(&event_source);

        return true;
}

Orchestrator *orch_new(uint16_t port, const char *bus_service_name) {
        fprintf(stdout, "Creating Orchestrator...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = NULL;
        r = asprintf(&service_name, "%s", bus_service_name);
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }

        _cleanup_sd_bus_ sd_bus *user_dbus = user_bus_open(event);
        if (user_dbus == NULL) {
                fprintf(stderr, "Failed to open user dbus\n");
                return NULL;
        }
        r = sd_bus_request_name(user_dbus, service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                fprintf(stderr, "Failed to acquire service name on user dbus: %s\n", strerror(-r));
                return NULL;
        }

        Orchestrator *orch = malloc0(sizeof(Orchestrator));
        orch->accept_port = port;
        orch->user_bus_service_name = steal_pointer(&service_name);
        orch->event = steal_pointer(&event);
        orch->user_dbus = steal_pointer(&user_dbus);
        orch->peer_manager = peer_manager_new(orch->event);
        orch->peer_connection_source = NULL;

        bool successful = orch_setup_connection_handler(
                        orch, orch->accept_port, &peer_manager_accept_connection_request);
        if (!successful) {
                orch_unrefp(&orch);
                return NULL;
        }

        return orch;
}

void orch_unrefp(Orchestrator **orchestrator) {
        fprintf(stdout, "Freeing allocated memory of Orchestrator...\n");
        if (orchestrator == NULL || (*orchestrator) == NULL) {
                return;
        }
        if ((*orchestrator)->event != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Orchestrator...\n");
                sd_event_unrefp(&(*orchestrator)->event);
        }
        if ((*orchestrator)->user_bus_service_name != NULL) {
                fprintf(stdout, "Freeing allocated user_bus_service_name of Orchestrator...\n");
                free((*orchestrator)->user_bus_service_name);
        }
        if ((*orchestrator)->peer_connection_source != NULL) {
                fprintf(stdout, "Freeing allocated sd-event-source for connections of Orchestrator...\n");
                sd_event_source_unrefp(&(*orchestrator)->peer_connection_source);
        }
        if ((*orchestrator)->peer_manager != NULL) {
                fprintf(stdout, "Freeing allocated peer manager of Orchestrator...\n");
                peer_manager_unrefp(&(*orchestrator)->peer_manager);
        }
        if ((*orchestrator)->user_dbus != NULL) {
                fprintf(stdout, "Freeing allocated dbus to local user dbus...\n");
                sd_bus_unrefp(&(*orchestrator)->user_dbus);
        }

        free(*orchestrator);
}

bool orch_start(const Orchestrator *orchestrator) {
        fprintf(stdout, "Starting Orchestrator...\n");

        if (orchestrator == NULL) {
                return false;
        }
        if (orchestrator->event == NULL) {
                return false;
        }

        int r = 0;
        r = sd_event_loop(orchestrator->event);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool orch_stop(const Orchestrator *orchestrator) {
        fprintf(stdout, "Stopping Orchestrator...\n");

        if (orchestrator == NULL) {
                return false;
        }

        return true;
}
