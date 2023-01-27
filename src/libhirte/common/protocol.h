#pragma once

// NOLINTNEXTLINE#
#define HIRTE_DEFAULT_PORT 808 /* TODO: Pick a better default */

#define HIRTE_DBUS_NAME "org.containers.hirte"
#define HIRTE_NODE_DBUS_NAME "org.containers.hirte.Node"

#define HIRTE_OBJECT_PATH "/org/containers/hirte"

/* Public objects */
#define HIRTE_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH
#define NODE_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/node"

/* Public interfaces */
#define HIRTE_INTERFACE_BASE_NAME "org.containers.hirte"
#define MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Manager"
#define NODE_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Node"


/* Internal objects */
#define INTERNAL_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH "/internal"
#define INTERNAL_NODE_OBJECT_PATH HIRTE_OBJECT_PATH "/node"

/* Internal interfaces */
#define INTERNAL_MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Manager"
#define INTERNAL_NODE_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Node"

#define HIRTE_BUS_ERROR_OFFLINE "org.containers.hirte.Offline"

#define USEC_PER_SEC 1000000
#define HIRTE_DEFAULT_DBUS_TIMEOUT (USEC_PER_SEC * 30)
