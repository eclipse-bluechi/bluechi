/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <getopt.h>

#include "client.h"

#include "libbluechi/service/shutdown.h"

Client *new_client() {
        Client *client = malloc0(sizeof(Client));
        if (client == NULL) {
                return NULL;
        }

        client->ref_count = 1;

        return client;
}

void client_unref(Client *client) {
        client->ref_count--;
        if (client->ref_count != 0) {
                return;
        }

        if (client->api_bus != NULL) {
                sd_bus_unrefp(&client->api_bus);
        }
        if (client->object_path != NULL) {
                free_and_null(client->object_path);
        }

        free(client);
}

sd_bus_message *client_wait_for_job(Client *client, char *object_path) {
        int r = 0;

        assert(client->pending_job_name == NULL);
        client->pending_job_name = object_path;

        for (;;) {
                /* Process requests */
                r = sd_bus_process(client->api_bus, NULL);
                if (r < 0) {
                        fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
                        return NULL;
                }

                /* Did we get the result? */
                if (client->pending_job_result != NULL) {
                        return steal_pointer(&client->pending_job_result);
                }

                if (r > 0) { /* request processed, try to process another one */
                        continue;
                }

                /* Wait for the next request to process */
                r = sd_bus_wait(client->api_bus, (uint64_t) -1);
                if (r < 0) {
                        fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
                        return NULL;
                }
        }
}

int match_job_removed_signal(sd_bus_message *m, void *userdata, UNUSED sd_bus_error *error) {
        const char *job_path = NULL, *result = NULL, *node = NULL, *unit = NULL;
        uint32_t id = 0;
        int r = 0;
        Client *client = (Client *) userdata;

        if (client->pending_job_name == NULL) {
                return 0;
        }

        r = sd_bus_message_read(m, "uosss", &id, &job_path, &node, &unit, &result);
        if (r < 0) {
                fprintf(stderr, "Can't parse job result\n");
                return 0;
        }

        (void) sd_bus_message_rewind(m, true);

        if (streq(client->pending_job_name, job_path)) {
                client->pending_job_result = sd_bus_message_ref(m);
                return 1;
        }

        return 0;
}

int client_open_sd_bus(Client *client) {
        int r = 0;

#ifdef USE_USER_API_BUS
        r = sd_bus_open_user(&(client->api_bus));
#else
        r = sd_bus_open_system(&(client->api_bus));
#endif
        if (r < 0) {
                fprintf(stderr, "Failed to connect to api bus: %s\n", strerror(-r));
                return r;
        }

        return 0;
}

int client_create_message_new_method_call(
                Client *client, const char *node_name, const char *member, sd_bus_message **new_message) {
        int r = 0;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;

        free_and_null(client->object_path);
        r = assemble_object_path_string(NODE_OBJECT_PATH_PREFIX, node_name, &client->object_path);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_new_method_call(
                        client->api_bus,
                        &outgoing_message,
                        BC_INTERFACE_BASE_NAME,
                        client->object_path,
                        NODE_INTERFACE,
                        member);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new method call: %s\n", strerror(-r));
                return r;
        }

        *new_message = sd_bus_message_ref(outgoing_message);
        return 0;
}

int client_start_event_loop(Client *client) {
        _cleanup_sd_event_ sd_event *event = NULL;
        int r = sd_event_default(&event);
        if (r < 0) {
                fprintf(stderr, "Failed to create event loop: %s", strerror(-r));
                return r;
        }

        r = sd_bus_attach_event(client->api_bus, event, SD_EVENT_PRIORITY_NORMAL);
        if (r < 0) {
                fprintf(stderr, "Failed to attach api bus to event: %s", strerror(-r));
                return r;
        }

        r = event_loop_add_shutdown_signals(event, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to add signals to agent event loop: %s", strerror(-r));
                return r;
        }

        r = sd_event_loop(event);
        if (r < 0) {
                fprintf(stderr, "Failed to start event loop: %s", strerror(-r));
                return r;
        }

        return 0;
}
