#include <arpa/inet.h>
#include <stdio.h>

#include "libhirte/bus/peer-bus.h"
#include "libhirte/bus/systemd-bus.h"
#include "libhirte/bus/user-bus.h"
#include "libhirte/common/common.h"
#include "libhirte/common/opt.h"
#include "libhirte/common/parse-util.h"
#include "libhirte/ini/config.h"
#include "libhirte/service/shutdown.h"

#include "node.h"

Node *node_new(void) {
        fprintf(stdout, "Creating Node...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = strdup(NODE_SERVICE_DEFAULT_NAME);
        if (service_name == NULL) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }

        Node *n = malloc0(sizeof(Node));
        n->ref_count = 1;
        n->event = steal_pointer(&event);
        n->user_bus_service_name = steal_pointer(&service_name);
        n->port = NODE_DEFAULT_PORT;

        return n;
}

Node *node_ref(Node *node) {
        node->ref_count++;
        return node;
}

void node_unref(Node *node) {
        node->ref_count--;
        if (node->ref_count != 0) {
                return;
        }

        free(node->name);
        free(node->host);
        free(node->orch_addr);
        free(node->user_bus_service_name);

        if (node->event != NULL) {
                sd_event_unrefp(&node->event);
        }

        if (node->peer_dbus != NULL) {
                sd_bus_unrefp(&node->peer_dbus);
        }
        if (node->user_dbus != NULL) {
                sd_bus_unrefp(&node->user_dbus);
        }
        if (node->systemd_dbus != NULL) {
                sd_bus_unrefp(&node->systemd_dbus);
        }

        free(node);
}

void node_unrefp(Node **nodep) {
        if (nodep && *nodep) {
                node_unref(*nodep);
                *nodep = NULL;
        }
}

bool node_set_port(Node *node, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                fprintf(stderr, "Invalid port format '%s'\n", port_s);
                return false;
        }
        node->port = port;
        return true;
}

bool node_set_host(Node *node, const char *host) {
        char *dup = strdup(host);
        if (dup == NULL) {
                fprintf(stderr, "Out of memory\n");
                return false;
        }
        free(node->host);
        node->host = dup;
        return true;
}

bool node_set_name(Node *node, const char *name) {
        char *dup = strdup(name);
        if (dup == NULL) {
                fprintf(stderr, "Out of memory\n");
                return false;
        }

        free(node->name);
        node->name = dup;
        return true;
}

bool node_parse_config(Node *node, const char *configfile) {
        _cleanup_hashmap_ struct hashmap *ini_hashmap = NULL;
        struct hashmap *node_hashmap = NULL;
        const char *name = NULL, *host = NULL, *port = NULL;

        ini_hashmap = parsing_ini_file(configfile);
        if (ini_hashmap == NULL) {
                return false;
        }

        node_hashmap = hashmap_get(ini_hashmap, "Node");

        if (node_hashmap == NULL) {
                return true;
        }

        name = hashmap_get(node_hashmap, "Name");
        if (name) {
                if (!node_set_name(node, name)) {
                        return false;
                }
        }

        host = hashmap_get(node_hashmap, "Host");
        if (host) {
                if (!node_set_host(node, host)) {
                        return false;
                }
        }

        port = hashmap_get(node_hashmap, "Port");
        if (port) {
                if (!node_set_port(node, port)) {
                        return false;
                }
        }

        return true;
}

bool node_start(Node *node) {
        struct sockaddr_in host;
        int r = 0;

        fprintf(stdout, "Starting Node...\n");

        if (node == NULL) {
                return false;
        }

        memset(&host, 0, sizeof(host));
        host.sin_family = AF_INET;
        host.sin_port = htons(node->port);

        if (node->name == NULL) {
                fprintf(stderr, "No name specified\n");
                return false;
        }

        if (node->host == NULL) {
                fprintf(stderr, "No host specified\n");
                return false;
        }

        r = inet_pton(AF_INET, node->host, &host.sin_addr);
        if (r < 1) {
                fprintf(stderr, "Invalid host option '%s'\n", optarg);
                return false;
        }

        node->orch_addr = assemble_tcp_address(&host);
        if (node->orch_addr == NULL) {
                return false;
        }

        node->user_dbus = user_bus_open(node->event);
        if (node->user_dbus == NULL) {
                fprintf(stderr, "Failed to open user dbus\n");
                return false;
        }

        r = sd_bus_request_name(node->user_dbus, node->user_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                fprintf(stderr, "Failed to acquire service name on user dbus: %s\n", strerror(-r));
                return false;
        }

        node->systemd_dbus = systemd_bus_open(node->event);
        if (node->systemd_dbus == NULL) {
                fprintf(stderr, "Failed to open systemd dbus\n");
                return false;
        }

        node->peer_dbus = peer_bus_open(node->event, "peer-bus-to-orchestrator", node->orch_addr);
        if (node->peer_dbus == NULL) {
                fprintf(stderr, "Failed to open peer dbus\n");
                return false;
        }

        r = shutdown_service_register(node->user_dbus, node->event);
        if (r < 0) {
                fprintf(stderr, "Failed to register shutdown service\n");
                return false;
        }

        r = event_loop_add_shutdown_signals(node->event);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to node event loop\n");
                return false;
        }

        r = sd_event_loop(node->event);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool node_stop(Node *node) {
        fprintf(stdout, "Stopping Node...\n");

        if (node == NULL) {
                return false;
        }

        return true;
}
