#pragma once

// NOLINTNEXTLINE#
#define HIRTE_DEFAULT_PORT 808 /* TODO: Pick a better default */

#define HIRTE_DBUS_NAME "org.containers.hirte"
#define HIRTE_NODE_DBUS_NAME "org.containers.hirte.Node"

#define HIRTE_OBJECT_PATH "/org/containers/hirte"
#define HIRTE_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH
#define HIRTE_INTERNAL_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH "/internal"
#define NODE_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/node"

#define HIRTE_INTERFACE_BASE_NAME "org.containers.hirte"
#define MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Manager"
#define INTERNAL_MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Manager"
#define NODE_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Node"
