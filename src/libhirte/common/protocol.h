/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include <errno.h>

#define HIRTE_DEFAULT_PORT 842
#define HIRTE_DEFAULT_HOST "127.0.0.1"

#define HIRTE_DBUS_NAME "org.containers.hirte"
#define HIRTE_AGENT_DBUS_NAME "org.containers.hirte.Agent"

#define HIRTE_OBJECT_PATH "/org/containers/hirte"

/* Public objects */
#define HIRTE_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH
#define HIRTE_AGENT_OBJECT_PATH HIRTE_OBJECT_PATH

/* Public interfaces */
#define HIRTE_INTERFACE_BASE_NAME "org.containers.hirte"
#define MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Manager"
#define AGENT_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Agent"
#define NODE_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Node"
#define JOB_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Job"
#define MONITOR_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Monitor"
#define METRICS_INTERFACE HIRTE_INTERFACE_BASE_NAME ".Metrics"

#define NODE_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/node"
#define JOB_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/job"
#define MONITOR_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/monitor"
#define METRICS_OBJECT_PATH HIRTE_OBJECT_PATH "/metrics"

/* Internal objects */
#define INTERNAL_MANAGER_OBJECT_PATH HIRTE_OBJECT_PATH "/internal"
#define INTERNAL_AGENT_OBJECT_PATH HIRTE_OBJECT_PATH "/internal/agent"
#define INTERNAL_PROXY_OBJECT_PATH_PREFIX HIRTE_OBJECT_PATH "/internal/proxy"
#define INTERNAL_AGENT_METRICS_OBJECT_PATH INTERNAL_AGENT_OBJECT_PATH "/metrics"

/* Internal interfaces */
#define INTERNAL_MANAGER_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Manager"
#define INTERNAL_AGENT_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Agent"
#define INTERNAL_PROXY_INTERFACE HIRTE_INTERFACE_BASE_NAME ".internal.Proxy"
#define INTERNAL_AGENT_METRICS_INTERFACE INTERNAL_AGENT_INTERFACE ".Metrics"

#define HIRTE_BUS_ERROR_OFFLINE "org.containers.hirte.Offline"
#define HIRTE_BUS_ERROR_NO_SUCH_SUBSCRIPTION "org.containers.hirte.NoSuchSubscription"
#define HIRTE_BUS_ERROR_ACTIVATION_FAILED "org.containers.hirte.ActivationFailed"

/* Systemd protocol */

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_IFACE "org.freedesktop.systemd1.Manager"
#define SYSTEMD_UNIT_IFACE "org.freedesktop.systemd1.Unit"

#define USEC_PER_SEC 1000000
#define USEC_PER_MSEC 1000
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

typedef enum UnitActiveState {
        UNIT_ACTIVE,
        UNIT_RELOADING,
        UNIT_INACTIVE,
        UNIT_FAILED,
        UNIT_ACTIVATING,
        UNIT_DEACTIVATING,
        UNIT_MAINTENANCE,
        _UNIT_ACTIVE_STATE_MAX,
        _UNIT_ACTIVE_STATE_INVALID = -EINVAL,
} UnitActiveState;

const char *active_state_to_string(UnitActiveState s);
UnitActiveState active_state_from_string(const char *s);

char *get_hostname();

/* Agent to Hirte heartbeat signals */

// Application-level heartbeat set at 2 seconds.
#define AGENT_HEARTBEAT_INTERVAL_MSEC (2000)
#define AGENT_HEARTBEAT_SIGNAL_NAME "Heartbeat"

/* Constants */
#define SYMBOL_WILDCARD "*"
#define SYMBOL_GLOB_ALL '*'
#define SYMBOL_GLOB_ONE '?'
