/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "libhirte/bus/utils.h"
#include "libhirte/common/common.h"

#include "types.h"

struct Client {
        int ref_count;

        char *op;
        int opargc;
        char **opargv;

        sd_bus *api_bus;
        char *object_path;

        char *pending_job_name;
        sd_bus_message *pending_job_result;
};

Client *new_client(char *op, int opargc, char **opargv);
Client *client_ref(Client *client);
void client_unref(Client *client);

int client_call_manager(Client *client);
int print_client_usage(char *argv);

DEFINE_CLEANUP_FUNC(Client, client_unref)
#define _cleanup_client_ _cleanup_(client_unrefp)
