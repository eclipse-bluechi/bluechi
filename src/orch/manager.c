#include <stdio.h>
#include <string.h>

#include "libhirte/bus/peer-bus.h"
#include "libhirte/bus/user-bus.h"
#include "libhirte/common/common.h"
#include "libhirte/common/parse-util.h"
#include "libhirte/ini/config.h"
#include "libhirte/service/shutdown.h"
#include "libhirte/socket.h"

#include "managednode.h"
#include "manager.h"

Manager *manager_new(void) {
        fprintf(stdout, "Creating Manager...\n");

        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;
        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return NULL;
        }

        _cleanup_free_ char *service_name = strdup(HIRTE_DBUS_NAME);
        if (service_name == NULL) {
                fprintf(stderr, "Out of memory\n");
                return NULL;
        }

        Manager *manager = malloc0(sizeof(Manager));
        if (manager != NULL) {
                manager->port = HIRTE_DEFAULT_PORT;
                manager->user_bus_service_name = steal_pointer(&service_name);
                manager->event = steal_pointer(&event);
                LIST_HEAD_INIT(manager->nodes);
        }

        return manager;
}

Manager *manager_ref(Manager *manager) {
        manager->ref_count++;
        return manager;
}

void manager_unref(Manager *manager) {
        manager->ref_count--;
        if (manager->ref_count != 0) {
                return;
        }

        if (manager->event != NULL) {
                sd_event_unrefp(&manager->event);
        }

        free(manager->user_bus_service_name);

        if (manager->node_connection_source != NULL) {
                sd_event_source_unrefp(&manager->node_connection_source);
        }
        if (manager->user_dbus != NULL) {
                sd_bus_unrefp(&manager->user_dbus);
        }

        free(manager);
}

void manager_unrefp(Manager **managerp) {
        if (managerp && *managerp) {
                manager_unref(*managerp);
                *managerp = NULL;
        }
}

ManagedNode *manager_find_node(Manager *manager, const char *name) {
        ManagedNode *node = NULL;

        LIST_FOREACH(nodes, node, manager->nodes) {
                if (node->name != NULL && strcmp(node->name, name) == 0) {
                        return node;
                }
        }

        return NULL;
}

void manager_remove_node(Manager *manager, ManagedNode *node) {
        LIST_REMOVE(nodes, manager->nodes, node);
        managed_node_unref(node);
}

bool manager_set_port(Manager *manager, const char *port_s) {
        uint16_t port = 0;

        if (!parse_port(port_s, &port)) {
                fprintf(stderr, "Invalid port format '%s'\n", port_s);
                return false;
        }
        manager->port = port;
        return true;
}

bool manager_parse_config(Manager *manager, const char *configfile) {
        _cleanup_hashmap_ struct hashmap *ini_hashmap = NULL;
        struct hashmap *manager_hashmap = NULL;
        const char *port = NULL;

        ini_hashmap = parsing_ini_file(configfile);
        if (ini_hashmap == NULL) {
                return false;
        }

        manager_hashmap = hashmap_get(ini_hashmap, "Manager");
        if (manager_hashmap == NULL) {
                return true;
        }

        port = hashmap_get(manager_hashmap, "Port");
        if (port) {
                if (!manager_set_port(manager, port)) {
                        return false;
                }
        }

        return true;
}

static int manager_accept_node_connection(
                UNUSED sd_event_source *source, int fd, UNUSED uint32_t revents, void *userdata) {
        Manager *manager = userdata;
        ManagedNode *node = NULL;
        _cleanup_fd_ int nfd = accept_tcp_connection_request(fd);

        if (nfd < 0) {
                return -1;
        }

        _cleanup_sd_bus_ sd_bus *dbus_server = peer_bus_open_server(
                        manager->event, "managed-node", HIRTE_DBUS_NAME, steal_fd(&nfd));
        if (dbus_server == NULL) {
                return -1;
        }

        node = managed_node_new(manager, dbus_server);
        if (dbus_server == NULL) {
                return -1;
        }

        LIST_APPEND(nodes, manager->nodes, node);

        return 0;
}

static bool manager_setup_node_connection_handler(Manager *manager) {
        int r = 0;
        _cleanup_sd_event_source_ sd_event_source *event_source = NULL;

        _cleanup_fd_ int accept_fd = create_tcp_socket(manager->port);
        if (accept_fd < 0) {
                return false;
        }

        r = sd_event_add_io(
                        manager->event, &event_source, accept_fd, EPOLLIN, manager_accept_node_connection, manager);
        if (r < 0) {
                fprintf(stderr, "Failed to add io event: %s\n", strerror(-r));
                return false;
        }
        r = sd_event_source_set_io_fd_own(event_source, true);
        if (r < 0) {
                fprintf(stderr, "Failed to set io fd own: %s\n", strerror(-r));
                return false;
        }

        // sd_event_set_io_fd_own takes care of closing accept_fd
        steal_fd(&accept_fd);

        (void) sd_event_source_set_description(event_source, "node-accept-socket");

        manager->node_connection_source = steal_pointer(&event_source);

        return true;
}

bool manager_start(Manager *manager) {
        fprintf(stdout, "Starting Manager...\n");

        if (manager == NULL) {
                return false;
        }

        manager->user_dbus = user_bus_open(manager->event);
        if (manager->user_dbus == NULL) {
                fprintf(stderr, "Failed to open user dbus\n");
                return false;
        }

        int r = sd_bus_request_name(
                        manager->user_dbus, manager->user_bus_service_name, SD_BUS_NAME_REPLACE_EXISTING);
        if (r < 0) {
                fprintf(stderr, "Failed to acquire service name on user dbus: %s\n", strerror(-r));
                return false;
        }

        if (!manager_setup_node_connection_handler(manager)) {
                return false;
        }

        r = shutdown_service_register(manager->user_dbus, manager->event);
        if (r < 0) {
                fprintf(stderr, "Failed to register shutdown service\n");
                return false;
        }

        r = event_loop_add_shutdown_signals(manager->event);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to manager event loop\n");
                return false;
        }

        r = sd_event_loop(manager->event);
        if (r < 0) {
                fprintf(stderr, "Starting event loop failed: %s\n", strerror(-r));
                return false;
        }

        return true;
}

bool manager_stop(UNUSED Manager *manager) {
        fprintf(stdout, "Stopping Manager...\n");

        if (manager == NULL) {
                return false;
        }

        return true;
}
