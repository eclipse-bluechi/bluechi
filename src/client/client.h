#pragma once

#include "libhirte/bus/utils.h"
#include "libhirte/common/common.h"

#include "printer.h"
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
        uint64_t job_time_millis;

        Printer printer;
};

Client *new_client(char *op, int opargc, char **opargv);
Client *client_ref(Client *client);
void client_unref(Client *client);

struct UnitList {
        int ref_count;

        LIST_FIELDS(UnitList, node_units);

        LIST_HEAD(UnitInfo, units);
};

UnitList *new_unit_list();
UnitList *unit_list_ref(UnitList *unit_list);
void unit_list_unref(UnitList *unit_list);

int client_call_manager(Client *client);

DEFINE_CLEANUP_FUNC(Client, client_unref)
#define _cleanup_client_ _cleanup_(client_unrefp)
DEFINE_CLEANUP_FUNC(UnitList, unit_list_unref)
#define _cleanup_unit_list_ _cleanup_(unit_list_unrefp)
