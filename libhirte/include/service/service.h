#pragma once

#define HIRTE_OBJECT_PATH "/org/containers/hirte"
#define HIRTE_INTERFACE_BASE_NAME "org.containers.hirte"

#define SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define SYSTEMD_OBJECT_PATH "/org/freedesktop/systemd1"
#define SYSTEMD_MANAGER_IFACE "org.freedesktop.systemd1.Manager"

char *assemble_interface_name(char *name_postfix);
