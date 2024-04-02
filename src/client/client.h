/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/bus/utils.h"
#include "libbluechi/common/common.h"

#include "types.h"

struct Client {
        int ref_count;

        sd_bus *api_bus;
        char *object_path;

        char *pending_job_name;
        sd_bus_message *pending_job_result;
};

Client *new_client();
void client_unref(Client *client);

int client_open_sd_bus(Client *client);

sd_bus_message *client_wait_for_job(Client *client, char *object_path);
int match_job_removed_signal(sd_bus_message *m, void *userdata, sd_bus_error *error);

DEFINE_CLEANUP_FUNC(Client, client_unref)
#define _cleanup_client_ _cleanup_(client_unrefp)

int client_create_message_new_method_call(
                Client *client, const char *node_name, const char *member, sd_bus_message **new_message);

int client_start_event_loop(Client *client);
