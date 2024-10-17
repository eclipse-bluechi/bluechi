/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "libbluechi/bus/bus.h"
#include "libbluechi/bus/utils.h"
#include "libbluechi/common/event-util.h"
#include "libbluechi/common/parse-util.h"
#include "libbluechi/service/shutdown.h"

#include "is-online.h"
#include "opt.h"

#define DBUS_SERVICE_NAME "org.freedesktop.DBus"
#define DBUS_SERVICE_PATH "/org/freedesktop/DBus"
#define DBUS_SERVICE_IFACE "org.freedesktop.DBus"
#define DBUS_METHOD_GETNAMEOWNER "GetNameOwner"


typedef struct ConnectionState {
        bool has_name;
        bool is_connected;
        bool is_switching_controller;
} ConnectionState;

typedef struct ParsedParams {
        long initial_wait;
        bool should_monitor;
        long switch_timeout;
        const char *service;
} ParsedParams;

typedef struct IsOnlineCli {
        sd_event *event;
        ConnectionState *state;
        ParsedParams *params;
} IsOnlineCli;

bool connection_state_is_online(ConnectionState *state) {
        return state->has_name && state->is_connected;
}

static int timer_callback(UNUSED sd_event_source *event_source, UNUSED uint64_t usec, void *userdata) {
        IsOnlineCli *is_online_cli = (IsOnlineCli *) userdata;
        if (connection_state_is_online(is_online_cli->state) && is_online_cli->params->should_monitor) {
                return 0;
        }
        return sd_event_exit(is_online_cli->event, 0);
}

static int setup_timer(IsOnlineCli *is_online_cli, uint64_t timeout) {
        _cleanup_(sd_event_source_unrefp) sd_event_source *event_source = NULL;
        int r = 0;

        r = event_reset_time_relative(
                        is_online_cli->event,
                        &event_source,
                        CLOCK_BOOTTIME,
                        timeout,
                        0,
                        timer_callback,
                        is_online_cli,
                        0,
                        "bluechi-is-online-timer-source",
                        false);
        if (r < 0) {
                fprintf(stderr, "Failed to reset agent heartbeat timer: %s\n", strerror(-r));
                return r;
        }

        return sd_event_source_set_floating(event_source, true);
}

static int parse_status_changed_from_property(sd_bus_message *m, char **ret_connection_status) {
        int r = 0;
        /* node status is always of type string */
        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
        if (r < 0) {
                fprintf(stderr, "Failed to enter content of variant type: %s\n", strerror(-r));
                return r;
        }
        char *status = NULL;
        r = sd_bus_message_read(m, "s", &status);
        if (r < 0) {
                fprintf(stderr, "Failed to read value of changed property: %s\n", strerror(-r));
                return r;
        }
        *ret_connection_status = strdup(status);

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                fprintf(stderr, "Failed to exit container: %s\n", strerror(-r));
                return r;
        }

        return 0;
}

static int handle_connection_state_changed(sd_bus_message *m, IsOnlineCli *is_online_cli) {
        int r = 0;
        _cleanup_free_ char *con_state = NULL;

        r = parse_status_changed_from_property(m, &con_state);
        if (r < 0) {
                return r;
        }
        if (con_state == NULL) {
                fprintf(stderr, "Received connection status change signal with missing 'Status' key.\n");
                return 0;
        }

        ConnectionState *state = is_online_cli->state;
        ParsedParams *params = is_online_cli->params;

        state->is_connected = (streq(con_state, "online") || streq(con_state, "up"));

        if (state->is_switching_controller && !connection_state_is_online(state)) {
                /* invalidate switching controller exception since we received the related disconnect */
                state->is_switching_controller = false;
                return 0;
        }

        if ((params->should_monitor && !connection_state_is_online(state)) ||
            (!params->should_monitor && params->initial_wait > 0 && connection_state_is_online(state))) {
                return sd_event_exit(is_online_cli->event, 0);
        }

        return 0;
}

static int handle_controller_address_changed(IsOnlineCli *is_online_cli) {
        if (is_online_cli->params->switch_timeout > 0) {
                setup_timer(is_online_cli, is_online_cli->params->switch_timeout * USEC_PER_MSEC);
                is_online_cli->state->is_switching_controller = true;
        }
        return 0;
}

static int on_properties_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        (void) sd_bus_message_skip(m, "s");

        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                fprintf(stderr, "Failed to read changed properties: %s\n", strerror(-r));
                return r;
        }

        IsOnlineCli *is_online_cli = (IsOnlineCli *) userdata;
        for (;;) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                if (r <= 0) {
                        break;
                }

                const char *key = NULL;
                r = sd_bus_message_read(m, "s", &key);
                if (r < 0) {
                        fprintf(stderr, "Failed to read next unit changed property: %s\n", strerror(-r));
                        return r;
                }
                if (r == 0) {
                        break;
                }

                /* only process property changes for the node status or the controller address */
                if (streq(key, "Status")) {
                        handle_connection_state_changed(m, is_online_cli);
                } else if (streq(key, "ControllerAddress")) {
                        handle_controller_address_changed(is_online_cli);
                }
        }

        return sd_bus_message_exit_container(m);
}

static int on_name_owner_changed(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        const char *name = NULL;
        const char *old = NULL;
        const char *new = NULL;
        int r = sd_bus_message_read(m, "sss", &name, &old, &new);
        if (r < 0) {
                return r;
        }

        IsOnlineCli *is_online_cli = (IsOnlineCli *) userdata;
        if (name != NULL && new != NULL && streq(name, is_online_cli->params->service) && !streq(new, "")) {
                is_online_cli->state->has_name = true;
        }

        return 0;
}

static int setup_event_loop(sd_bus *bus, const char *service_name, const char *path, IsOnlineCli *is_online_cli) {
        int r = 0;
        _cleanup_sd_event_ sd_event *event = NULL;

        r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach bus to event: %s\n", strerror(-r));
                return r;
        }

        r = event_loop_add_shutdown_signals(event, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to agent event loop: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_match_signal(
                        bus,
                        NULL,
                        service_name,
                        path,
                        "org.freedesktop.DBus.Properties",
                        "PropertiesChanged",
                        on_properties_changed,
                        is_online_cli);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for NodeConnectionStateChanged: %s\n", strerror(-r));
                return r;
        }
        r = sd_bus_match_signal(
                        bus,
                        NULL,
                        DBUS_SERVICE_NAME,
                        DBUS_SERVICE_PATH,
                        DBUS_SERVICE_IFACE,
                        "NameOwnerChanged",
                        on_name_owner_changed,
                        is_online_cli);
        if (r < 0) {
                fprintf(stderr, "Failed to create callback for NameAcquired: %s\n", strerror(-r));
                return r;
        }

        is_online_cli->event = steal_pointer(&event);
        return 0;
}

static void start_monitoring(
                sd_bus *bus,
                const char *service_name,
                const char *path,
                ConnectionState *state,
                ParsedParams *params) {
        int r = 0;
        IsOnlineCli is_online_cli = {
                .state = state,
                .params = params,
                .event = NULL,
        };

        r = setup_event_loop(bus, service_name, path, &is_online_cli);
        if (r < 0) {
                return;
        }
        _cleanup_sd_event_ sd_event *cleanup = is_online_cli.event;

        if (is_online_cli.params->initial_wait > 0) {
                r = setup_timer(&is_online_cli, is_online_cli.params->initial_wait * USEC_PER_MSEC);
                if (r < 0) {
                        return;
                }
        }

        r = sd_event_loop(is_online_cli.event);
        if (r < 0) {
                fprintf(stderr, "Failed to start event loop: %s\n", strerror(-r));
                return;
        }
}

static bool is_online(sd_bus *bus, const char *service_name, const char *path, const char *iface) {
        _cleanup_sd_bus_error_ sd_bus_error prop_error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *prop_reply = NULL;

        int r = sd_bus_get_property(bus, service_name, path, iface, "Status", &prop_error, &prop_reply, "s");
        if (r < 0) {
                fprintf(stderr, "Failed to get property: %s\n", strerror(-r));
                return false;
        }

        const char *status = NULL;
        r = sd_bus_message_read(prop_reply, "s", &status);
        if (r < 0) {
                fprintf(stderr, "Failed to read property: %s\n", strerror(-r));
                return false;
        }

        return (status != NULL && (streq(status, "online") || streq(status, "up")));
}

static bool is_service_available(sd_bus *bus, const char *service_name) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;

        r = sd_bus_call_method(
                        bus,
                        DBUS_SERVICE_NAME,
                        DBUS_SERVICE_PATH,
                        DBUS_SERVICE_IFACE,
                        DBUS_METHOD_GETNAMEOWNER,
                        &error,
                        &result,
                        "s",
                        service_name);

        return r >= 0;
}

static int open_bus(sd_bus **bus) {
        int r = 0;

#ifdef USE_USER_API_BUS
        r = sd_bus_open_user(bus);
#else
        r = sd_bus_open_system(bus);
#endif
        if (r < 0) {
                fprintf(stderr, "Failed to connect to api bus: %s\n", strerror(-r));
                return r;
        }

        return 0;
}

static int run_is_online(Command *command, const char *service, const char *path, const char *interface) {
        int r = 0;
        bool should_monitor = false;
        long initial_wait = 0;
        long switch_timeout = 0;

        r = command_get_option_long(command, ARG_WAIT_SHORT, &initial_wait);
        if (r < 0) {
                fprintf(stderr, "Failed to parse --%s: %s\n", ARG_WAIT, strerror(-r));
                return -EINVAL;
        }
        r = command_get_option_long(command, ARG_SWITCH_CTRL_TIMEOUT_SHORT, &switch_timeout);
        if (r < 0) {
                fprintf(stderr, "Failed to parse --%s: %s\n", ARG_SWITCH_CTRL_TIMEOUT, strerror(-r));
                return -EINVAL;
        }
        should_monitor = command_flag_exists(command, ARG_MONITOR_SHORT);

        _cleanup_sd_bus_ sd_bus *bus = NULL;
        r = open_bus(&bus);
        if (r < 0) {
                fprintf(stderr, "Failed to open bus: %s\n", strerror(-r));
                return r;
        }

        ConnectionState state = {
                .has_name = false,
                .is_connected = false,
                .is_switching_controller = false,
        };
        ParsedParams params = {
                .initial_wait = initial_wait,
                .should_monitor = should_monitor,
                .switch_timeout = switch_timeout,
                .service = service,
        };

        state.has_name = is_service_available(bus, service);
        if (state.has_name) {
                state.is_connected = is_online(bus, service, path, interface);
        }

        if ((should_monitor && connection_state_is_online(&state)) ||
            (initial_wait > 0 && !connection_state_is_online(&state))) {
                start_monitoring(bus, service, path, &state, &params);
        }

        if (!connection_state_is_online(&state)) {
                fprintf(stderr, "%s is offline\n", service);
                return -1;
        }
        return 0;
}

int method_is_online_agent(Command *command, UNUSED void *userdata) {
        return run_is_online(command, BC_AGENT_DBUS_NAME, BC_AGENT_OBJECT_PATH, AGENT_INTERFACE);
}

int method_is_online_node(Command *command, UNUSED void *userdata) {
        int r = 0;
        const char *node_name = command->opargv[0];
        _cleanup_free_ char *node_path = NULL;
        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &node_path);
        if (r < 0) {
                fprintf(stderr, "Failed to assemble path for node '%s': %s\n", node_name, strerror(-r));
                return r;
        }

        return run_is_online(command, BC_DBUS_NAME, node_path, NODE_INTERFACE);
}

int method_is_online_system(Command *command, UNUSED void *userdata) {
        return run_is_online(command, BC_DBUS_NAME, BC_OBJECT_PATH, CONTROLLER_INTERFACE);
}
