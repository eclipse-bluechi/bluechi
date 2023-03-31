/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libhirte/common/cfg.h"
#include "libhirte/common/common.h"

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

        assert(null_value || strcmp(value, expected_value) == 0);
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

        assert(null_value || strcmp(value, expected_value) == 0);
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
        assert(strcmp(cfg_get_value(config, KEY), KEY_ORIG_VALUE) == 0);

        // add the same key into section 3 and the value of the key in global section and in section 2 shouldn't change
        _config_section_set_and_get(config, KEY, S3, KEY_S3_ORIG_VALUE, KEY_S3_ORIG_VALUE, false);
        assert(strcmp(cfg_get_value(config, KEY), KEY_ORIG_VALUE) == 0);
        assert(strcmp(cfg_s_get_value(config, KEY, S2), KEY_S2_ORIG_VALUE) == 0);

        // change the value of the key in global section and the value of the key in section 2 and 3 shouldn't change
        _config_set_and_get(config, KEY, KEY_NEW_VALUE, KEY_NEW_VALUE, false);
        assert(strcmp(cfg_s_get_value(config, KEY, S2), KEY_S2_ORIG_VALUE) == 0);
        assert(strcmp(cfg_s_get_value(config, KEY, S3), KEY_S3_ORIG_VALUE) == 0);

        // change the value of the key in section 2 and the value of the key in global section and section 3 shouldn't change
        _config_section_set_and_get(config, KEY, S2, KEY_S2_NEW_VALUE, KEY_S2_NEW_VALUE, false);
        assert(strcmp(cfg_get_value(config, KEY), KEY_NEW_VALUE) == 0);
        assert(strcmp(cfg_s_get_value(config, KEY, S3), KEY_S3_ORIG_VALUE) == 0);

        cfg_dispose(config);
}

void test_non_existent_option() {
        struct config *config = NULL;
        cfg_initialize(&config);

        assert(cfg_get_value(config, "non existent key") == NULL);

        cfg_dispose(config);
}

void test_parse_config_from_file() {
        const char *cfg_file_content =
                        "key1 = value1\n"
                        "key2 = value 2\n"
                        "# key3 = ignored\n"
                        "key4 = value 4\n"
                        "key5=\n";


        // check if tmp file was created and filled-in successfully
        char cfg_file_name[30];
        sprintf(cfg_file_name, "/tmp/cfg-file-%6d", rand() % 999999);
        FILE *cfg_file = fopen(cfg_file_name, "we");
        assert(cfg_file != NULL);
        fputs(cfg_file_content, cfg_file);
        fclose(cfg_file);

        struct config *config = NULL;
        cfg_initialize(&config);

        int result = cfg_load_from_file(config, cfg_file_name);
        assert(result == 0);

        assert(cfg_get_value(config, "key1") != NULL);
        assert(strcmp(cfg_get_value(config, "key1"), "value1") == 0);
        assert(cfg_get_value(config, "key2") != NULL);
        assert(strcmp(cfg_get_value(config, "key2"), "value 2") == 0);
        assert(cfg_get_value(config, "key3") == NULL);
        assert(cfg_get_value(config, "key4") != NULL);
        assert(strcmp(cfg_get_value(config, "key4"), "value 4") == 0);
        assert(cfg_get_value(config, "key5") == NULL);

        cfg_dispose(config);

        unlink(cfg_file_name);
}

void test_parse_sectioned_config_from_file() {
        const char *cfg_file_content =
                        "key = value1\n"
                        "[section2]\n"
                        "key = value 2\n"
                        "# key = ignored\n"
                        "[section 3]\n"
                        "# [ignored section]\n"
                        "key = value 3\n";


        // check if tmp file was created and filled-in successfully
        char cfg_file_name[35];
        sprintf(cfg_file_name, "/tmp/sect-cfg-file-%6d", rand() % 999999);
        FILE *cfg_file = fopen(cfg_file_name, "we");
        assert(cfg_file != NULL);
        fputs(cfg_file_content, cfg_file);
        fclose(cfg_file);

        struct config *config = NULL;
        cfg_initialize(&config);

        int result = cfg_load_from_file(config, cfg_file_name);
        assert(result == 0);

        // key value from the global section
        assert(cfg_get_value(config, "key") != NULL);
        assert(strcmp(cfg_get_value(config, "key"), "value1") == 0);

        // key value from section2
        assert(cfg_s_get_value(config, "section2", "key") != NULL);
        assert(strcmp(cfg_s_get_value(config, "section2", "key"), "value 2") == 0);

        // key value from section 3
        assert(cfg_s_get_value(config, "section 3", "key") != NULL);
        assert(strcmp(cfg_s_get_value(config, "section 3", "key"), "value 3") == 0);

        cfg_dispose(config);

        unlink(cfg_file_name);
}

void dispose_env() {
        unsetenv("HIRTE_LOG_LEVEL");
        unsetenv("HIRTE_LOG_TARGET");
        unsetenv("HIRTE_LOG_IS_QUIET");
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
        assert(strcmp(cfg_s_get_value(config, old_default_section, KEY), OLD_SECTION_VALUE1) == 0);

        // Change value in old_default_section and check the value didn't change in NEW_DEFAULT_SECTION
        _config_section_set_and_get(
                        config, old_default_section, KEY, OLD_SECTION_VALUE2, OLD_SECTION_VALUE2, false);
        assert(strcmp(cfg_s_get_value(config, NEW_DEFAULT_SECTION, KEY), NEW_SECTION_VALUE) == 0);

        free(old_default_section);
        cfg_dispose(config);
}

void test_parse_config_from_env() {
        const char *LOG_LEVEL_DEBUG = "DEBUG";
        const char *LOG_TARGET_STDERR = "stderr";
        const char *LOG_IS_QUIET_OFF = "false";

        struct config *config = NULL;
        cfg_initialize(&config);

        // initialize environment
        setenv("HIRTE_LOG_LEVEL", LOG_LEVEL_DEBUG, 1);
        setenv("HIRTE_LOG_TARGET", LOG_TARGET_STDERR, 1);
        setenv("HIRTE_LOG_IS_QUIET", LOG_IS_QUIET_OFF, 1);

        // parse config from environment
        cfg_load_from_env(config);

        const char *value = NULL;

        value = cfg_get_value(config, CFG_LOG_LEVEL);
        assert(value != NULL);
        assert(strcmp(value, LOG_LEVEL_DEBUG) == 0);

        value = cfg_get_value(config, CFG_LOG_TARGET);
        assert(value != NULL);
        assert(strcmp(value, LOG_TARGET_STDERR) == 0);

        value = cfg_get_value(config, CFG_LOG_IS_QUIET);
        assert(value != NULL);
        assert(strcmp(value, LOG_IS_QUIET_OFF) == 0);

        cfg_dispose(config);
        dispose_env();
}

void test_env_override_config() {
        const char *LOG_LEVEL_DEBUG = "DEBUG";
        const char *LOG_LEVEL_INFO = "INFO";
        const char *LOG_TARGET_STDERR = "stderr";
        const char *LOG_IS_QUIET_ON = "on";

        struct config *config = NULL;
        cfg_initialize(&config);

        // initialize default configuration
        cfg_set_value(config, CFG_LOG_LEVEL, LOG_LEVEL_DEBUG);
        cfg_set_value(config, CFG_LOG_TARGET, LOG_TARGET_STDERR);
        cfg_set_value(config, CFG_LOG_IS_QUIET, LOG_IS_QUIET_ON);

        // initialize environment
        // log level should be overridden
        setenv("HIRTE_LOG_LEVEL", LOG_LEVEL_INFO, 1);
        // empty string in shell is considered an empty value, so log target should not be overridden
        setenv("HIRTE_LOG_TARGET", "", 1);
        // log quiet is not defined in env, so it should not be overridden

        // parse config from environment
        cfg_load_from_env(config);

        const char *value = NULL;

        value = cfg_get_value(config, CFG_LOG_LEVEL);
        assert(value != NULL);
        assert(strcmp(value, LOG_LEVEL_INFO) == 0);

        value = cfg_get_value(config, CFG_LOG_TARGET);
        assert(value != NULL);
        assert(strcmp(value, LOG_TARGET_STDERR) == 0);

        value = cfg_get_value(config, CFG_LOG_IS_QUIET);
        assert(value != NULL);
        assert(strcmp(value, LOG_IS_QUIET_ON) == 0);

        cfg_dispose(config);
        dispose_env();
}

void test_config_dump() {
        struct config *config = NULL;
        cfg_initialize(&config);

        _config_set_and_get(config, "key1", "value1", "value1", false);
        _config_set_and_get(config, "key2", "value4", "value4", false);

        const char *expected_cfg_dump = "key1=value1\nkey2=value4\n";

        _cleanup_free_ const char *cfg = cfg_dump(config);
        assert(streq(cfg, expected_cfg_dump));

        cfg_dispose(config);
}

int main() {
        test_config_set_get();
        test_config_set_get_bool();
        test_config_section_set_get();
        test_non_existent_option();
        test_parse_config_from_file();
        test_parse_sectioned_config_from_file();
        test_default_section();
        test_parse_config_from_env();
        test_env_override_config();
        test_config_dump();

        return EXIT_SUCCESS;
}