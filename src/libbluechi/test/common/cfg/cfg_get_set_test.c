/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"

void _config_set_and_get(
                struct config *config,
                const char *option,
                const char *value,
                const char *expected_value,
                bool null_value) {
        assert(option != NULL);

        assert(cfg_set_value(config, option, value) == 0);

        const char *found_value = cfg_get_value(config, option);

        // found value should not be NULL unless we stored an option with NULL value
        assert(null_value || found_value != NULL);

        assert(null_value || streq(value, expected_value));
}

void test_config_set_get() {
        struct config *config = NULL;
        cfg_initialize(&config);

        _config_set_and_get(config, "key1", "value1", "value1", false);
        _config_set_and_get(config, "key2", NULL, NULL, true);
        _config_set_and_get(config, "key1", "value3", "value3", false);
        _config_set_and_get(config, "key2", "value4", "value4", false);

        cfg_dispose(config);
}

void _config_set_get_bool(struct config *config, const char *option, const char *value, bool expected_value) {
        assert(option != NULL);

        assert(cfg_set_value(config, option, value) == 0);

        assert(cfg_get_bool_value(config, option) == expected_value);
}

void test_config_set_get_bool() {
        struct config *config = NULL;
        cfg_initialize(&config);

        _config_set_get_bool(config, "key01", "true", true);
        _config_set_get_bool(config, "key02", "True", true);
        _config_set_get_bool(config, "key03", "yes", true);
        _config_set_get_bool(config, "key04", "YeS", true);
        _config_set_get_bool(config, "key05", "on", true);
        _config_set_get_bool(config, "key06", "ON", true);
        _config_set_get_bool(config, "key07", NULL, false);
        _config_set_get_bool(config, "key08", "bla", false);
        _config_set_get_bool(config, "key07", "false", false);
        _config_set_get_bool(config, "key07", "FALSE", false);
        _config_set_get_bool(config, "key07", "nO", false);
        _config_set_get_bool(config, "key07", "No", false);
        _config_set_get_bool(config, "key07", "OfF", false);
        _config_set_get_bool(config, "key07", "off", false);

        cfg_dispose(config);
}

void _config_section_set_and_get(
                struct config *config,
                const char *section,
                const char *option,
                const char *value,
                const char *expected_value,
                bool null_value) {
        assert(option != NULL);

        assert(cfg_s_set_value(config, section, option, value) == 0);

        const char *found_value = cfg_s_get_value(config, section, option);

        // found value should not be NULL unless we stored an option with NULL value
        assert(null_value || found_value != NULL);

        assert(null_value || streq(value, expected_value));
}

void test_config_section_set_get() {
        const char *KEY = "key";
        const char *S2 = "s_2";
        const char *S3 = "s_3";
        const char *KEY_ORIG_VALUE = "v_1";
        const char *KEY_NEW_VALUE = "new_v_1";
        const char *KEY_S2_ORIG_VALUE = "v_2";
        const char *KEY_S2_NEW_VALUE = "new_v_2";
        const char *KEY_S3_ORIG_VALUE = "v_3";
        struct config *config = NULL;
        cfg_initialize(&config);

        _config_set_and_get(config, KEY, KEY_ORIG_VALUE, KEY_ORIG_VALUE, false);

        // add the same key into section 2 and the value of the key in global section shouldn't change
        _config_section_set_and_get(config, KEY, S2, KEY_S2_ORIG_VALUE, KEY_S2_ORIG_VALUE, false);
        assert(streq(cfg_get_value(config, KEY), KEY_ORIG_VALUE));

        // add the same key into section 3 and the value of the key in global section and in section 2 shouldn't change
        _config_section_set_and_get(config, KEY, S3, KEY_S3_ORIG_VALUE, KEY_S3_ORIG_VALUE, false);
        assert(streq(cfg_get_value(config, KEY), KEY_ORIG_VALUE));
        assert(streq(cfg_s_get_value(config, KEY, S2), KEY_S2_ORIG_VALUE));

        // change the value of the key in global section and the value of the key in section 2 and 3 shouldn't change
        _config_set_and_get(config, KEY, KEY_NEW_VALUE, KEY_NEW_VALUE, false);
        assert(streq(cfg_s_get_value(config, KEY, S2), KEY_S2_ORIG_VALUE));
        assert(streq(cfg_s_get_value(config, KEY, S3), KEY_S3_ORIG_VALUE));

        // change the value of the key in section 2 and the value of the key in global section and section 3 shouldn't change
        _config_section_set_and_get(config, KEY, S2, KEY_S2_NEW_VALUE, KEY_S2_NEW_VALUE, false);
        assert(streq(cfg_get_value(config, KEY), KEY_NEW_VALUE));
        assert(streq(cfg_s_get_value(config, KEY, S3), KEY_S3_ORIG_VALUE));

        cfg_dispose(config);
}

void test_non_existent_option() {
        struct config *config = NULL;
        cfg_initialize(&config);

        assert(cfg_get_value(config, "non existent key") == NULL);

        cfg_dispose(config);
}


void test_default_section() {
        const char *NEW_DEFAULT_SECTION = "my_section";
        const char *KEY = "key1";
        const char *OLD_SECTION_VALUE1 = "value1";
        const char *NEW_SECTION_VALUE = "value2";
        const char *OLD_SECTION_VALUE2 = "value3";
        struct config *config = NULL;
        cfg_initialize(&config);

        char *old_default_section = strdup(cfg_get_default_section(config));

        // Test setting and getting value using old_default_section
        _config_set_and_get(config, KEY, OLD_SECTION_VALUE1, OLD_SECTION_VALUE1, false);

        cfg_set_default_section(config, NEW_DEFAULT_SECTION);

        // Test setting and getting value using NEW_DEFAULT_SECTION
        _config_set_and_get(config, KEY, NEW_SECTION_VALUE, NEW_SECTION_VALUE, false);

        // Check that values in old_default_section didn't change
        assert(streq(cfg_s_get_value(config, old_default_section, KEY), OLD_SECTION_VALUE1));

        // Change value in old_default_section and check the value didn't change in NEW_DEFAULT_SECTION
        _config_section_set_and_get(
                        config, old_default_section, KEY, OLD_SECTION_VALUE2, OLD_SECTION_VALUE2, false);
        assert(streq(cfg_s_get_value(config, NEW_DEFAULT_SECTION, KEY), NEW_SECTION_VALUE));

        free(old_default_section);
        cfg_dispose(config);
}

void test_config_dump() {
        struct config *config = NULL;
        cfg_initialize(&config);

        _config_set_and_get(config, "key1", "value1", "value1", false);
        _config_set_and_get(config, "key2", "value4", "value4", false);

        _cleanup_free_ const char *cfg = cfg_dump(config);
        assert(strstr(cfg, "key1=value1\n") != NULL);
        assert(strstr(cfg, "key2=value4\n") != NULL);

        cfg_dispose(config);
}


int main() {
        test_config_set_get();
        test_config_set_get_bool();
        test_config_section_set_get();
        test_non_existent_option();
        test_default_section();
        test_config_dump();

        return EXIT_SUCCESS;
}
