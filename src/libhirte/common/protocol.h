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
#define JOB_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Job"

#define NODE_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/node"
#define JOB_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/job"

/* Internal objects */
#define INTERNAL_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH "/internal"
#define INTERNAL_AGENT_OBJECT_PATH HIRTE_OBJECT_PATH "/internal/agent"

/* Internal interfaces */
#define INTERNAL_MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Manager"
#define INTERNAL_AGENT_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Agent"

#define HIRTE_BUS_ERROR_OFFLINE "org.containers.hirte.Offline"

/* Systemd protocol */

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_IFACE "org.freedesktop.systemd1.Manager"

#define USEC_PER_SEC 1000000
#define HIRTE_DEFAULT_DBUS_TIMEOUT ((uint64_t) (USEC_PER_SEC * 30))

/* Typestrings */

#define UNIT_INFO_TYPESTRING "ssssssouso"
#define UNIT_INFO_STRUCT_TYPESTRING "(" UNIT_INFO_TYPESTRING ")"
#define UNIT_INFO_STRUCT_ARRAY_TYPESTRING "a" UNIT_INFO_STRUCT_TYPESTRING

#define NODE_AND_UNIT_INFO_TYPESTRING "s" UNIT_INFO_TYPESTRING
#define NODE_AND_UNIT_INFO_STRUCT_TYPESTRING "(" NODE_AND_UNIT_INFO_TYPESTRING ")"
#define NODE_AND_UNIT_INFO_STRUCT_ARRAY_TYPESTRING "a" NODE_AND_UNIT_INFO_STRUCT_TYPESTRING


typedef enum JobState {
        JOB_WAITING,
        JOB_RUNNING,
        JOB_INVALID = -1,
} JobState;

const char *job_state_to_string(JobState s);
JobState job_state_from_string(const char *s);
