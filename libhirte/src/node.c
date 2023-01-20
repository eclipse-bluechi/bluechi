#include <stdio.h>

#include "../include/bus/peer-bus.h"
#include "../include/bus/systemd-bus.h"
#include "../include/bus/user-bus.h"
#include "../include/common/common.h"
#include "../include/node.h"
#include "./common/dbus.h"

static int node_signal_handler(
                sd_event_source *event_source, UNUSED const struct signalfd_siginfo *si, UNUSED void *userdata) {
        // Do whatever cleanup is needed here.

        sd_event *event = sd_event_source_get_event(event_source);
        sd_event_exit(event, 0);

        return 0;
}

static int node_setup_signal_handler(sd_event *event) {
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
        r = sd_event_add_signal(event, NULL, SIGTERM, node_signal_handler, NULL);
        if (r < 0) {
                fprintf(stderr, "sd_event_add_signal() failed: %m\n");
                return -1;
        }

        return 0;
}

Node *node_new(const NodeParams *params) {
        fprintf(stdout, "Creating Node...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %m\n");
                return NULL;
        }

        r = node_setup_signal_handler(event);
        if (r < 0) {
                fprintf(stderr, "node_setup_signal_handler() failed\n");
                return false;
        }

        char *orch_addr = assemble_tcp_address(params->orch_addr);
        if (orch_addr == NULL) {
                return NULL;
        }

        _cleanup_sd_bus_ sd_bus *user_dbus = user_bus_open(event);
        if (user_dbus == NULL) {
                fprintf(stderr, "Failed to open user dbus\n");
                return false;
        }
        _cleanup_sd_bus_ sd_bus *systemd_dbus = systemd_bus_open(event);
        if (systemd_dbus == NULL) {
                fprintf(stderr, "Failed to open systemd dbus\n");
                return false;
        }
        _cleanup_sd_bus_ sd_bus *peer_dbus = peer_bus_open(event, "peer-bus-to-orchestrator", orch_addr);
        if (peer_dbus == NULL) {
                fprintf(stderr, "Failed to open peer dbus\n");
                return false;
        }

        Node *n = malloc0(sizeof(Node));
        n->orch_addr = orch_addr;
        n->event_loop = steal_pointer(&event);
        n->user_dbus = steal_pointer(&user_dbus);
        n->systemd_dbus = steal_pointer(&systemd_dbus);
        n->peer_dbus = steal_pointer(&peer_dbus);

        return n;
}

void node_unrefp(Node **node) {
        fprintf(stdout, "Freeing allocated memory of Node...\n");
        if (node == NULL || (*node) == NULL) {
                return;
        }
        if ((*node)->event_loop != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Node...\n");
                sd_event_unrefp(&(*node)->event_loop);
        }
        if ((*node)->orch_addr != NULL) {
                fprintf(stdout, "Freeing allocated orch_addr of Node...\n");
                free((*node)->orch_addr);
        }
        if ((*node)->peer_dbus != NULL) {
                fprintf(stdout, "Freeing allocated peer dbus to Orchestrator...\n");
                sd_bus_unrefp(&(*node)->peer_dbus);
        }
        if ((*node)->user_dbus != NULL) {
                fprintf(stdout, "Freeing allocated dbus to local user dbus...\n");
                sd_bus_unrefp(&(*node)->user_dbus);
        }
        if ((*node)->systemd_dbus != NULL) {
                fprintf(stdout, "Freeing allocated dbus to local systemd dbus...\n");
                sd_bus_unrefp(&(*node)->systemd_dbus);
        }

        free(*node);
}

bool node_start(Node *node) {
        fprintf(stdout, "Starting Node...\n");

        if (node == NULL) {
                return false;
        }

        int r = 0;
        r = sd_event_loop(node->event_loop);
        if (r < 0) {
                fprintf(stderr, "Starting node event loop failed: %m\n");
                return false;
        }

        fprintf(stdout, "Exited node event loop()\n");

        return true;
}

bool node_stop(const Node *node) {
        fprintf(stdout, "Stopping Node...\n");

        if (node == NULL) {
                return false;
        }

        return true;
}
