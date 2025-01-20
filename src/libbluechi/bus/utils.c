/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "libbluechi/common/string-util.h"

#include "utils.h"

int bus_parse_properties_foreach(sd_bus_message *m, bus_property_cb cb, void *userdata) {
        bool stop = false;
        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                return r;
        }
        while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
                const char *key = NULL;

                r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &key);
                if (r < 0) {
                        return r;
                }

                char type = 0;
                const char *value_type = NULL;
                r = sd_bus_message_peek_type(m, &type, &value_type);
                if (r < 0) {
                        return r;
                }

                r = (*cb)(key, value_type, m, userdata);
                if (r < 0) {
                        return r;
                } else if (r == 1) {
                        stop = true;
                } else if (r == 2) {
                        r = sd_bus_message_skip(m, "v");
                        if (r < 0) {
                                return r;
                        }
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        return r;
                }

                if (stop) {
                        break;
                }
        }
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                return r;
        }

        return stop ? 1 : 0;
}

struct ParsePropertyData {
        const char *name;
        const char **value;
};

static int bus_parse_property_string_cb(
                const char *key, const char *value_type, sd_bus_message *m, void *userdata) {
        struct ParsePropertyData *data = userdata;

        if (!streq(data->name, key)) {
                return 2; /* skip item */
        }

        if (!streq(value_type, "s")) {
                return -EIO; /* Wrong type */
        }

        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, value_type);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, data->value);
        if (r < 0) {
                return r;
        }

        r = sd_bus_message_exit_container(m);
        if (r < 0) {
                return r;
        }

        return 1;
}

int bus_parse_property_string(sd_bus_message *m, const char *name, const char **value) {
        struct ParsePropertyData data = { name, value };
        int r = bus_parse_properties_foreach(m, bus_parse_property_string_cb, &data);
        if (r < 0) {
                return r;
        }
        return (r == 0) ? -ENOENT : 0;
}

UnitInfo *new_unit() {
        _cleanup_unit_ UnitInfo *unit = malloc0(sizeof(UnitInfo));
        if (unit == NULL) {
                return NULL;
        }

        unit->ref_count = 1;
        LIST_INIT(units, unit);

        return steal_pointer(&unit);
}

UnitInfo *unit_ref(UnitInfo *unit) {
        unit->ref_count++;
        return unit;
}

void unit_unref(UnitInfo *unit) {
        unit->ref_count--;
        if (unit->ref_count != 0) {
                return;
        }

        free_and_null(unit->node);
        free_and_null(unit->id);
        free_and_null(unit->description);
        free_and_null(unit->load_state);
        free_and_null(unit->active_state);
        free_and_null(unit->sub_state);
        free_and_null(unit->following);
        free_and_null(unit->unit_path);
        free_and_null(unit->job_type);
        free_and_null(unit->job_path);

        free(unit);
}

/*
 * Copied from systemd/bus-unit-util.c
 */
int bus_parse_unit_info(sd_bus_message *message, UnitInfo *u) {
        int r = 0;
        char *id = NULL, *description = NULL, *load_state = NULL, *active_state = NULL;
        char *sub_state = NULL, *following = NULL, *unit_path = NULL;
        int job_id = 0;
        char *job_type = NULL, *job_path = NULL;

        assert(message);
        assert(u);

        r = sd_bus_message_read(
                        message,
                        UNIT_INFO_STRUCT_TYPESTRING,
                        &id,
                        &description,
                        &load_state,
                        &active_state,
                        &sub_state,
                        &following,
                        &unit_path,
                        &job_id,
                        &job_type,
                        &job_path);

        if (r <= 0) {
                return r;
        }

        u->node = NULL;
        u->id = strdup(id);
        u->description = strdup(description);
        u->load_state = strdup(load_state);
        u->active_state = strdup(active_state);
        u->sub_state = strdup(sub_state);
        u->following = strdup(following);
        u->unit_path = strdup(unit_path);
        u->job_id = job_id;
        u->job_type = strdup(job_type);
        u->job_path = strdup(job_path);

        return r;
}

UnitFileInfo *new_unit_file() {
        _cleanup_unit_file_ UnitFileInfo *unit_file = malloc0(sizeof(UnitFileInfo));
        if (unit_file == NULL) {
                return NULL;
        }

        unit_file->ref_count = 1;
        LIST_INIT(unit_files, unit_file);

        return steal_pointer(&unit_file);
}

UnitFileInfo *unit_file_ref(UnitFileInfo *unit_file) {
        unit_file->ref_count++;
        return unit_file;
}

void unit_file_unref(UnitFileInfo *unit_file) {
        unit_file->ref_count--;
        if (unit_file->ref_count != 0) {
                return;
        }

        free_and_null(unit_file->node);
        free_and_null(unit_file->unit_path);
        free_and_null(unit_file->enablement_status);

        free(unit_file);
}

int bus_parse_unit_file_info(sd_bus_message *m, UnitFileInfo *unit_file) {
        int r = 0;
        char *unit_path = NULL;
        char *enablement_status = NULL;

        assert(m);
        assert(unit_file);

        r = sd_bus_message_read(m, UNIT_FILE_INFO_STRUCT_TYPESTRING, &unit_path, &enablement_status);
        if (r <= 0) {
                return r;
        }

        unit_file->node = NULL;
        unit_file->unit_path = strdup(unit_path);
        unit_file->enablement_status = strdup(enablement_status);

        return r;
}

int assemble_object_path_string(const char *prefix, const char *name, char **res) {
        _cleanup_free_ char *escaped = bus_path_escape(name);
        if (escaped == NULL) {
                return -ENOMEM;
        }
        return asprintf(res, "%s/%s", prefix, escaped);
}

// Disabling -Wunterminated-string-initialization temporarily.
// hexchar uses the table array only for mapping an integer to
// a hexadecimal char, no need for it to be NULL terminated.
#if __GNUC__ >= 15
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wunterminated-string-initialization"
#endif
static char hexchar(int x) {
        static const char table[16] = "0123456789abcdef";
        return table[(unsigned int) x % sizeof(table)];
}
#if __GNUC__ >= 15
#        pragma GCC diagnostic pop
#endif

char *bus_path_escape(const char *s) {

        if (*s == 0) {
                return strdup("_");
        }

        char *r = malloc(strlen(s) * 3 + 1);
        if (r == NULL) {
                return NULL;
        }

        char *t = r;
        for (const char *f = s; *f; f++) {
                /* Escape everything that is not a-zA-Z0-9. We also escape 0-9 if it's the first character */
                if (!ascii_isalpha(*f) && !(f > s && ascii_isdigit(*f))) {
                        unsigned char c = *f;
                        *(t++) = '_';
                        *(t++) = hexchar(c >> 4U);
                        *(t++) = hexchar(c);
                } else {
                        *(t++) = *f;
                }
        }

        *t = 0;

        return r;
}

int bus_socket_set_options(sd_bus *bus, SocketOptions *opts) {
        int fd = sd_bus_get_fd(bus);
        if (fd < 0) {
                return fd;
        }

        return socket_set_options(fd, opts);
}

/*
 * Copied from libsystemd/sd-bus/bus-internal.c service_name_is_valid and adjusted to
 * exclude the well-known service names. Also does not support '_' and '-' characters.
 */
bool bus_id_is_valid(const char *name) {
        if (isempty(name) || name[0] != ':') {
                return false;
        }

        const char *i = name + 1;
        bool dot = true;
        bool found_dot = false;
        for (; *i; i++) {
                if (*i == '.') {
                        if (dot) {
                                return false;
                        }
                        found_dot = true;
                        dot = true;
                        continue;
                }
                dot = false;

                if (!ascii_isalpha(*i) && !ascii_isdigit(*i)) {
                        return false;
                }
        }

        if (i - name > SD_BUS_MAXIMUM_NAME_LENGTH) {
                return false;
        }

        if (dot || !found_dot) {
                return false;
        }

        return true;
}
