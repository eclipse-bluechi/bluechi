#include <errno.h>
#include <stdio.h>

#include "utils.h"

int bus_parse_property_string(sd_bus_message *m, const char *name, const char **value) {
        bool found = false;
        int r = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
        if (r < 0) {
                return r;
        }
        while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv")) > 0) {
                const char *member = NULL;
                const char *contents = NULL;

                r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &member);
                if (r < 0) {
                        return r;
                }

                if (streq(name, member)) {
                        r = sd_bus_message_peek_type(m, NULL, &contents);
                        if (r < 0) {
                                return r;
                        }

                        if (!streq(contents, "s")) {
                                return -EIO; /* Wrong type */
                        }

                        r = sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "s");
                        if (r < 0) {
                                return r;
                        }

                        r = sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, value);
                        if (r < 0) {
                                return r;
                        }

                        found = true;
                        r = sd_bus_message_exit_container(m);
                        if (r < 0) {
                                return r;
                        }
                } else {
                        r = sd_bus_message_skip(m, "v");
                        if (r < 0) {
                                return r;
                        }
                }

                r = sd_bus_message_exit_container(m);
                if (r < 0) {
                        return r;
                }

                if (found) {
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

        if (found) {
                return 0;
        }
        return -ENOENT;
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

        free(unit);
}

/*
 * Copied from systemd/bus-unit-util.c
 */
int bus_parse_unit_info(sd_bus_message *message, UnitInfo *u) {
        assert(message);
        assert(u);

        u->node = NULL;

        return sd_bus_message_read(
                        message,
                        UNIT_INFO_STRUCT_TYPESTRING,
                        &u->id,
                        &u->description,
                        &u->load_state,
                        &u->active_state,
                        &u->sub_state,
                        &u->following,
                        &u->unit_path,
                        &u->job_id,
                        &u->job_type,
                        &u->job_path);
}

int assemble_object_path_string(const char *prefix, const char *name, char **res) {
        /* TODO: Should escape the name if needed */
        int r = asprintf(res, "%s/%s", prefix, name);
        if (r < 0) {
                fprintf(stderr, "Out of memory\n");
        }
        return r;
}
