#include <stdio.h>
#include <string.h>

#include "../include/bus/user-bus.h"
#include "../include/common/common.h"
#include "../include/orchestrator.h"
#include "../include/orchestrator/peer-manager.h"
#include "../include/socket.h"
#include "./common/dbus.h"
#include "../include/common/common.h"

static int orch_signal_handler(sd_event_source *event_source,
                               UNUSED const struct signalfd_siginfo *si,
                               UNUSED void *userdata)
{
        // Do whatever cleanup is needed here.

        sd_event *event = sd_event_source_get_event(event_source);
        sd_event_exit(event, 0);

        return 0;
}

static int orch_setup_signal_handler(sd_event *event)
{
        sigset_t sigset;
        int r = 0;

        // Block this thread from handling SIGTERM so it can be handled by the
        // the event loop instead.
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
        r = sigprocmask(SIG_BLOCK, &sigset, NULL);
        if (r < 0) {
                fprintf(stderr, "sigprocmask() failed: %m\n");
                return -1;
        }

        // Add SIGTERM as an event source in the event loop.
        r = sd_event_add_signal(event, NULL, SIGTERM, orch_signal_handler, NULL);
        if (r < 0) {
                fprintf(stderr, "sd_event_add_signal() failed: %m\n");
                return -1;
        }

        return 0;
}

static bool orch_setup_connection_handler(
                Orchestrator *orch, uint16_t listen_port, sd_event_io_handler_t connection_callback) {
        if (orch == NULL) {
                fprintf(stderr, "Orchestrator is NULL\n");
                return false;
        }
        if (orch->event_loop == NULL) {
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
                        orch->event_loop,
                        &event_source,
                        accept_fd,
                        EPOLLIN,
                        connection_callback,
                        orch->peer_manager);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %m\n");
                return false;
        }
        // sd_event_add_io takes care of closing accept_fd, setting it to -1 avoids closing it multiple times
        // NOLINTNEXTLINE(clang-analyzer-deadcode.DeadStores)
        accept_fd = -1;

        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %m\n");
                return false;
        }
        (void) sd_event_source_set_description(event_source, "master-socket");

        orch->peer_connection_source = steal_pointer(&event_source);

        return true;
}

Orchestrator *orch_new(const OrchestratorParams *params) {
        fprintf(stdout, "Creating Orchestrator...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %m\n");
                return NULL;
        }

        r = orch_setup_signal_handler(event);
        if (r < 0) {
                fprintf(stderr, "orch_setup_signal_handler() failed\n");
                return false;
        }

        _cleanup_sd_bus_ sd_bus *user_dbus = user_bus_open(event);
        if (user_dbus == NULL) {
                fprintf(stderr, "Failed to open user dbus\n");
                return false;
        }

        Orchestrator *orch = malloc0(sizeof(Orchestrator));
        orch->accept_port = params->port;
        orch->event_loop = steal_pointer(&event);
        orch->user_dbus = steal_pointer(&user_dbus);
        orch->peer_manager = peer_manager_new(orch->event_loop);
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
        if ((*orchestrator)->event_loop != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Orchestrator...\n");
                sd_event_unrefp(&(*orchestrator)->event_loop);
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
        if (orchestrator->event_loop == NULL) {
                return false;
        }

        int r = 0;
        r = sd_event_loop(orchestrator->event_loop);
        if (r < 0) {
                fprintf(stderr, "Starting orch event loop failed: %m\n");
                return false;
        } 

        fprintf(stdout, "Exited orch event loop()\n");

        return true;
}


bool orch_stop(const Orchestrator *orchestrator) {
        fprintf(stdout, "Stopping Orchestrator...\n");

        if (orchestrator == NULL) {
                return false;
        }

        return true;
}
