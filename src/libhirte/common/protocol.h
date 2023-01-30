#pragma once

// NOLINTNEXTLINE#
#define HIRTE_DEFAULT_PORT 808 /* TODO: Pick a better default */

#define HIRTE_DBUS_NAME "org.containers.hirte"
#define HIRTE_AGENT_DBUS_NAME "org.containers.hirte.Agent"

#define HIRTE_OBJECT_PATH "/org/containers/hirte"

/* Public objects */
#define HIRTE_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH

/* Public interfaces */
#define HIRTE_INTERFACE_BASE_NAME "org.containers.hirte"
#define MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Manager"
#define NODE_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Node"

#define NODE_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/node"

/* Internal objects */
#define INTERNAL_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH "/internal"
#define INTERNAL_AGENT_OBJECT_PATH HIRTE_OBJECT_PATH "/internal/agent"

/* Internal interfaces */
#define INTERNAL_MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Manager"
#define INTERNAL_AGENT_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Agent"

#define HIRTE_BUS_ERROR_OFFLINE "org.containers.hirte.Offline"

#define USEC_PER_SEC 1000000
#define HIRTE_DEFAULT_DBUS_TIMEOUT ((uint64_t) (USEC_PER_SEC * 30))
