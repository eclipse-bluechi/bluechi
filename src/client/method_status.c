/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "libbluechi/bus/utils.h"
#include "libbluechi/common/common.h"

#include "client.h"
#include "method_status.h"

typedef struct unit_info_t {
        const char *id;
        const char *load_state;
        const char *active_state;
        const char *freezer_state;
        const char *sub_state;
        const char *unit_file_state;
} unit_info_t;

struct bus_properties_map {
        const char *member;
        size_t offset;
};

static const struct bus_properties_map property_map[] = {
        { "Id", offsetof(unit_info_t, id) },
        { "LoadState", offsetof(unit_info_t, load_state) },
        { "ActiveState", offsetof(unit_info_t, active_state) },
        { "FreezerState", offsetof(unit_info_t, freezer_state) },
        { "SubState", offsetof(unit_info_t, sub_state) },
        { "UnitFileState", offsetof(unit_info_t, unit_file_state) },
        {}
};

static int get_property_map(sd_bus_message *m, const struct bus_properties_map **prop) {
        int r = 0;
        const char *member = NULL;
        int i = 0;

        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &member);
        if (r < 0) {
                fprintf(stderr, "Failed to read the name of the property: %s\n", strerror(-r));
                return r;
        }

        for (i = 0; property_map[i].member; i++) {
                if (streq(property_map[i].member, member)) {
                        *prop = &property_map[i];
                        break;
                }
        }

        return 0;
}

static int read_property_value(sd_bus_message *m, const struct bus_properties_map *prop, unit_info_t *unit_info) {
        int r = 0;
        char type = 0;
        const char *contents = NULL;
        const char *s = NULL;
        const char **v = (const char **) ((uint8_t *) unit_info + prop->offset);

        r = sd_bus_message_peek_type(m, NULL, &contents);
        if (r < 0) {
                fprintf(stderr, "Failed to peek into the type of the property: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, contents);
        if (r < 0) {
                fprintf(stderr, "Failed to enter into the container of the property: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_message_peek_type(m, &type, NULL);
        if (r < 0) {
                fprintf(stderr, "Failed to get the type of the property: %s\n", strerror(-r));
                return r;
        }

        if (type != SD_BUS_TYPE_STRING) {
                fprintf(stderr, "Currently only string types are expected\n");
                return -EINVAL;
        }

        r = sd_bus_message_read_basic(m, type, &s);
        if (r < 0) {
                fprintf(stderr, "Failed to get the value of the property: %s\n", strerror(-r));
                return r;
        }

        if (isempty(s)) {
                s = NULL;
        }

        *v = s;

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                fprintf(stderr, "Failed to exit from the container of the property: %s\n", strerror(-r));
        }

        return r;
}

static int process_dict_entry(sd_bus_message *m, unit_info_t *unit_info) {
        int r = 0;
        const struct bus_properties_map *prop = NULL;

        r = get_property_map(m, &prop);
        if (r < 0) {
                return r;
        }

        if (prop) {
                r = read_property_value(m, prop, unit_info);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to get the value for member %s - %s\n",
                                prop->member,
                                strerror(-r));
                }
        } else {
                r = sd_bus_message_skip(m, "v");
                if (r < 0) {
                        fprintf(stderr, "Failed to skip the property container: %s\n", strerror(-r));
                }
        }

        return r;
}

static int parse_unit_status_response_from_message(sd_bus_message *m, unit_info_t *unit_info) {
        int r = 0;

        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                fprintf(stderr, "Failed to open the strings array container: %s\n", strerror(-r));
                return r;
        }

        for (;;) {
                r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                if (r < 0) {
                        fprintf(stderr, "Failed to enter the property container: %s\n", strerror(-r));
                        return r;
                }

                if (r == 0) {
                        break;
                }

                r = process_dict_entry(m, unit_info);
                if (r < 0) {
                        break;
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        fprintf(stderr, "Failed to exit the property container: %s\n", strerror(-r));
                        break;
                }
        }

        return r;
}

#define PRINT_TAB_SIZE 8
static void print_info_header(size_t name_col_width) {
        size_t i = 0;

        fprintf(stderr, "UNIT");
        for (i = PRINT_TAB_SIZE + name_col_width; i > PRINT_TAB_SIZE; i -= PRINT_TAB_SIZE) {
                fprintf(stderr, "\t");
        }

        fprintf(stderr, "| LOADED\t| ACTIVE\t| SUBSTATE\t| FREEZERSTATE\t| ENABLED\t|\n");
        for (i = PRINT_TAB_SIZE + name_col_width; i > PRINT_TAB_SIZE; i -= PRINT_TAB_SIZE) {
                fprintf(stderr, "--------");
        }
        fprintf(stderr, "----------------");
        fprintf(stderr, "----------------");
        fprintf(stderr, "----------------");
        fprintf(stderr, "----------------");
        fprintf(stderr, "----------------\n");
}

#define PRINT_AND_ALIGN(x)                                       \
        do {                                                     \
                fprintf(stderr, "| %s\t", unit_info->x);         \
                if (!unit_info->x || strlen(unit_info->x) < 6) { \
                        fprintf(stderr, "\t");                   \
                }                                                \
        } while (0)


static void print_unit_info(unit_info_t *unit_info, size_t name_col_width) {
        size_t i = 0;

        if (unit_info->load_state && streq(unit_info->load_state, "not-found")) {
                fprintf(stderr, "Unit %s could not be found.\n", unit_info->id);
                return;
        }

        fprintf(stderr, "%s", unit_info->id);
        name_col_width -= unit_info->id ? strlen(unit_info->id) : 0;
        name_col_width += PRINT_TAB_SIZE;
        for (i = PRINT_TAB_SIZE + name_col_width; i > PRINT_TAB_SIZE; i -= PRINT_TAB_SIZE) {
                fprintf(stderr, "\t");
        }

        PRINT_AND_ALIGN(load_state);
        PRINT_AND_ALIGN(active_state);
        PRINT_AND_ALIGN(sub_state);
        PRINT_AND_ALIGN(freezer_state);
        PRINT_AND_ALIGN(unit_file_state);

        fprintf(stderr, "|\n");
}

static size_t get_max_name_len(char **units, size_t units_count) {
        size_t i = 0;
        size_t max_unit_name_len = 0;

        for (i = 0; i < units_count; i++) {
                size_t unit_name_len = strlen(units[i]);
                max_unit_name_len = max_unit_name_len > unit_name_len ? max_unit_name_len : unit_name_len;
        }

        return max_unit_name_len;
}

static int get_status_unit_on(Client *client, char *node_name, char *unit_name, size_t name_col_width) {
        int r = 0;
        _cleanup_sd_bus_error_ sd_bus_error error = SD_BUS_ERROR_NULL;
        _cleanup_sd_bus_message_ sd_bus_message *result = NULL;
        _cleanup_sd_bus_message_ sd_bus_message *outgoing_message = NULL;
        unit_info_t unit_info = { 0 };

        r = create_message_new_method_call(client, node_name, "GetUnitProperties", &outgoing_message);
        if (r < 0) {
                fprintf(stderr, "Failed to create a new message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_message_append(outgoing_message, "ss", unit_name, "");
        if (r < 0) {
                fprintf(stderr, "Failed to append runtime to the message: %s\n", strerror(-r));
                return r;
        }

        r = sd_bus_call(client->api_bus, outgoing_message, BC_DEFAULT_DBUS_TIMEOUT, &error, &result);
        if (r < 0) {
                fprintf(stderr, "Failed to issue call: %s\n", error.message);
                return r;
        }

        r = parse_unit_status_response_from_message(result, &unit_info);
        if (r < 0) {
                fprintf(stderr, "Failed to parse the response strings array: %s\n", error.message);
                return r;
        }

        print_unit_info(&unit_info, name_col_width);

        return 0;
}

int method_status_unit_on(Client *client, char *node_name, char **units, size_t units_count) {
        unsigned i = 0;

        size_t max_name_len = get_max_name_len(units, units_count);

        print_info_header(max_name_len);
        for (i = 0; i < units_count; i++) {
                int r = get_status_unit_on(client, node_name, units[i], max_name_len);
                if (r < 0) {
                        fprintf(stderr,
                                "Failed to get status of unit %s on node %s - %s",
                                units[i],
                                node_name,
                                strerror(-r));
                        return r;
                }
        }

        return 0;
}
