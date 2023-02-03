#include <errno.h>
#include <stdio.h>

#include "libhirte/common/common.h"
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
