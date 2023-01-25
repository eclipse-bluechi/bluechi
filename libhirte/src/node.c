#include <stdio.h>

#include "./bus/peer-bus.h"
#include "./bus/systemd-bus.h"
#include "./bus/user-bus.h"
#include "./common/dbus.h"
#include "node.h"

Node *node_new(const struct sockaddr_in *peer_addr, const char *bus_service_name) {
        fprintf(stdout, "Creating Node...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *orch_addr = assemble_tcp_address(peer_addr);
        if (orch_addr == NULL) {
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

        _cleanup_sd_bus_ sd_bus *systemd_dbus = systemd_bus_open(event);
        if (systemd_dbus == NULL) {
                fprintf(stderr, "Failed to open systemd dbus\n");
                return NULL;
        }

        _cleanup_sd_bus_ sd_bus *peer_dbus = peer_bus_open(event, "peer-bus-to-orchestrator", orch_addr);
        if (peer_dbus == NULL) {
                fprintf(stderr, "Failed to open peer dbus\n");
                return NULL;
        }

        Node *n = malloc0(sizeof(Node));
        n->orch_addr = steal_pointer(&orch_addr);
        n->user_bus_service_name = steal_pointer(&service_name);
        n->event = steal_pointer(&event);
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
        if ((*node)->event != NULL) {
                fprintf(stdout, "Freeing allocated sd-event of Node...\n");
                sd_event_unrefp(&(*node)->event);
        }
        if ((*node)->orch_addr != NULL) {
                fprintf(stdout, "Freeing allocated orch_addr of Node...\n");
                free((*node)->orch_addr);
        }
        if ((*node)->user_bus_service_name != NULL) {
                fprintf(stdout, "Freeing allocated user_bus_service_name of Node...\n");
                free((*node)->user_bus_service_name);
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
        r = sd_event_loop(node->event);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool node_stop(const Node *node) {
        fprintf(stdout, "Stopping Node...\n");

        if (node == NULL) {
                return false;
        }

        return true;
}
