/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <errno.h>

#include "time-util.h"

/* Configuration defaults */

/* Enable using remote control for BlueChi via TCP */
#define CONTROLLER_DEFAULT_USE_TCP "true"
/* Disable using local control for BlueChi via Unix Domain Socket */
#define CONTROLLER_DEFAULT_USE_UDS "true"
/* Default TCP connection information */
#define BC_DEFAULT_PORT "842"
#define BC_DEFAULT_HOST "127.0.0.1"
/* Number of seconds idle before sending keepalive packets. Value is set to socket option TCP_KEEPIDLE. */
#define BC_DEFAULT_TCP_KEEPALIVE_TIME "1"
/* Number of seconds idle between each keepalive packet. Value is set to socket option TCP_KEEPINTVL. */
#define BC_DEFAULT_TCP_KEEPALIVE_INTERVAL "1"
/* Number of keepalive packets without ACK till connection is dropped. Value is set to socket option TCP_KEEPCNT. */
#define BC_DEFAULT_TCP_KEEPALIVE_COUNT "6"
/* Enable extended reliable error message passing. Sets the socket option IP_RECVERR. */
#define BC_DEFAULT_IP_RECEIVE_ERROR "true"
/* High-level timeout for DBus calls */
#define BC_DEFAULT_DBUS_TIMEOUT ((uint64_t) (USEC_PER_SEC * 30))
/* Application-level heartbeat interval */
#define AGENT_DEFAULT_HEARTBEAT_INTERVAL_MSEC "2000"
#define AGENT_DEFAULT_CONTROLLER_HEARTBEAT_THRESHOLD_MSEC "0"
#define CONTROLLER_DEFAULT_HEARTBEAT_INTERVAL_MSEC "0"
#define CONTROLLER_DEFAULT_NODE_HEARTBEAT_THRESHOLD_MSEC "6000"
/* Number of connection retries until logs are silenced */
#define AGENT_DEFAULT_CONNECTION_RETRY_COUNT_UNTIL_QUIET "10"


/* BlueChi DBus service names */
#define BC_DBUS_NAME "org.eclipse.bluechi"
#define BC_AGENT_DBUS_NAME "org.eclipse.bluechi.Agent"

/* Root object path */
#define BC_OBJECT_PATH "/org/eclipse/bluechi"

/* Public objects */
#define BC_CONTROLLER_OBJECT_PATH BC_OBJECT_PATH
#define BC_AGENT_OBJECT_PATH BC_OBJECT_PATH

/* Public interfaces */
#define BC_INTERFACE_BASE_NAME "org.eclipse.bluechi"
#define CONTROLLER_INTERFACE BC_INTERFACE_BASE_NAME ".Controller"
#define AGENT_INTERFACE BC_INTERFACE_BASE_NAME ".Agent"
#define NODE_INTERFACE BC_INTERFACE_BASE_NAME ".Node"
#define JOB_INTERFACE BC_INTERFACE_BASE_NAME ".Job"
#define MONITOR_INTERFACE BC_INTERFACE_BASE_NAME ".Monitor"
#define METRICS_INTERFACE BC_INTERFACE_BASE_NAME ".Metrics"

#define NODE_OBJECT_PATH_PREFIX BC_OBJECT_PATH "/node"
#define JOB_OBJECT_PATH_PREFIX BC_OBJECT_PATH "/job"
#define MONITOR_OBJECT_PATH_PREFIX BC_OBJECT_PATH "/monitor"
#define METRICS_OBJECT_PATH BC_OBJECT_PATH "/metrics"

/* Internal objects */
#define INTERNAL_CONTROLLER_OBJECT_PATH BC_OBJECT_PATH "/internal"
#define INTERNAL_AGENT_OBJECT_PATH BC_OBJECT_PATH "/internal/agent"
#define INTERNAL_PROXY_OBJECT_PATH_PREFIX BC_OBJECT_PATH "/internal/proxy"
#define INTERNAL_AGENT_METRICS_OBJECT_PATH INTERNAL_AGENT_OBJECT_PATH "/metrics"

/* Internal interfaces */
#define INTERNAL_CONTROLLER_INTERFACE BC_INTERFACE_BASE_NAME ".internal.Controller"
#define INTERNAL_AGENT_INTERFACE BC_INTERFACE_BASE_NAME ".internal.Agent"
#define INTERNAL_PROXY_INTERFACE BC_INTERFACE_BASE_NAME ".internal.Proxy"
#define INTERNAL_AGENT_METRICS_INTERFACE INTERNAL_AGENT_INTERFACE ".Metrics"

#define BC_BUS_ERROR_OFFLINE "org.eclipse.bluechi.Offline"
#define BC_BUS_ERROR_NO_SUCH_SUBSCRIPTION "org.eclipse.bluechi.NoSuchSubscription"
#define BC_BUS_ERROR_ACTIVATION_FAILED "org.eclipse.bluechi.ActivationFailed"

/* Systemd protocol */
#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_IFACE "org.freedesktop.systemd1.Manager"
#define SYSTEMD_UNIT_IFACE "org.freedesktop.systemd1.Unit"
#define SYSTEMD_JOB_IFACE "org.freedesktop.systemd1.Job"

/* Signal names */
#define AGENT_HEARTBEAT_SIGNAL_NAME "Heartbeat"
#define CONTROLLER_HEARTBEAT_SIGNAL_NAME "Heartbeat"

/* Typestrings */
#define UNIT_INFO_TYPESTRING "ssssssouso"
#define UNIT_INFO_STRUCT_TYPESTRING "(" UNIT_INFO_TYPESTRING ")"
#define UNIT_INFO_STRUCT_ARRAY_TYPESTRING "a" UNIT_INFO_STRUCT_TYPESTRING

#define NODE_AND_UNIT_INFO_TYPESTRING "s" UNIT_INFO_STRUCT_ARRAY_TYPESTRING
#define NODE_AND_UNIT_INFO_DICT_TYPESTRING "{" NODE_AND_UNIT_INFO_TYPESTRING "}"
#define NODE_AND_UNIT_INFO_DICT_ARRAY_TYPESTRING "a" NODE_AND_UNIT_INFO_DICT_TYPESTRING

#define UNIT_FILE_INFO_TYPESTRING "ss"
#define UNIT_FILE_INFO_STRUCT_TYPESTRING "(" UNIT_FILE_INFO_TYPESTRING ")"
#define UNIT_FILE_INFO_STRUCT_ARRAY_TYPESTRING "a" UNIT_FILE_INFO_STRUCT_TYPESTRING

#define NODE_AND_UNIT_FILE_INFO_TYPESTRING "s" UNIT_FILE_INFO_STRUCT_ARRAY_TYPESTRING
#define NODE_AND_UNIT_FILE_INFO_DICT_TYPESTRING "{" NODE_AND_UNIT_FILE_INFO_TYPESTRING "}"
#define NODE_AND_UNIT_FILE_INFO_DICT_ARRAY_TYPESTRING "a" NODE_AND_UNIT_FILE_INFO_DICT_TYPESTRING

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
