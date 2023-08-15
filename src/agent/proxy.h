/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include "libbluechi/common/common.h"

#include "types.h"

struct ProxyService {
        int ref_count;
        uint32_t id;

        Agent *agent; /* weak ref */

        char *node_name;
        char *unit_name;
        char *local_service_name;

        sd_bus_message *request_message;

        bool sent_new_proxy; /* We told manager about the proxy */
        bool dont_stop_proxy;
        bool sent_successful_ready;

        sd_bus_slot *export_slot;
        char *object_path;

        LIST_FIELDS(ProxyService, proxy_services); /* List in Agent */
};

ProxyService *proxy_service_new(
                Agent *agent,
                const char *local_service_name,
                const char *node_name,
                const char *unit_name,
                sd_bus_message *request_message);
ProxyService *proxy_service_ref(ProxyService *proxy);
void proxy_service_unref(ProxyService *proxy);

bool proxy_service_export(ProxyService *proxy);
void proxy_service_unexport(ProxyService *proxy);

int proxy_service_emit_proxy_new(ProxyService *proxy);
int proxy_service_emit_proxy_removed(ProxyService *proxy);

DEFINE_CLEANUP_FUNC(ProxyService, proxy_service_unref);
#define _cleanup_proxy_service_ _cleanup_(proxy_service_unrefp)
