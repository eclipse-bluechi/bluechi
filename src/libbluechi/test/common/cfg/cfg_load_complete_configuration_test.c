/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"

#define MAX_CUSTOM_CONFIGS 2
#define MAX_EXPECTED_ENTRIES 3

typedef struct cfg_test_input {
        char *default_config_content;
        char *custom_config_content;
        char *custom_configs[MAX_CUSTOM_CONFIGS];
} cfg_test_input;

typedef struct cfg_test_expected_entry {
        char *key;
        char *value;
} cfg_test_expected_entry;

typedef struct cfg_test_param {
        char *test_case_name;
        cfg_test_input input;

        int expected_ret;
        cfg_test_expected_entry expected_entries[MAX_EXPECTED_ENTRIES];
} cfg_test_param;

struct cfg_test_param test_data[] = {
        /* basic single config file tests */
        { "no config files",
          { NULL, NULL, { NULL, NULL } },
          0,
          { { NULL, NULL }, { NULL, NULL }, { NULL, NULL } } },
        { "only default config file",
          { "NodeName = laptop\n", NULL, { NULL, NULL } },
          0,
          { { "NodeName", "laptop" }, { NULL, NULL }, { NULL, NULL } } },
        { "only custom config file",
          { NULL, "NodeName = laptop\n", { NULL, NULL } },
          0,
          { { "NodeName", "laptop" }, { NULL, NULL }, { NULL, NULL } } },
        { "only file in confd",
          { NULL, "NodeName = laptop\n", { "NodeName = laptop\n", NULL } },
          0,
          { { "NodeName", "laptop" }, { NULL, NULL }, { NULL, NULL } } },

        /* testing of loading order and overwrite */
        { "with custom config file",
          { "NodeName = laptop\n", "NodeName = pi\n", { NULL, NULL } },
          0,
          { { "NodeName", "pi" }, { NULL, NULL }, { NULL, NULL } } },
        { "with config file in confd",
          { "NodeName = laptop\n", "NodeName = pi\n", { "NodeName = vm\n", NULL } },
          0,
          { { "NodeName", "vm" }, { NULL, NULL }, { NULL, NULL } } },
        { "with multiple config files in confd",
          { "NodeName = laptop\n", "NodeName = pi\n", { "NodeName = vm\n", "NodeName = container\n" } },
          0,
          { { "NodeName", "container" }, { NULL, NULL }, { NULL, NULL } } },

        /* testing of loading order, overwrite and merges */
        { "with multiple, duplicate keys",
          { "NodeName = laptop\nLogLevel=INFO\n",
            "NodeName = pi\n",
            { "LogLevel=DEBUG\n", "LogTarget=stderr\n" } },
          0,
          { { "NodeName", "pi" }, { "LogLevel", "DEBUG" }, { "LogTarget", "stderr" } } },

        /* testing invalid entries */
        { "with invalid value in default config file",
          { "NodeName 0 laptop\n", "NodeName = pi\n", { "LogLevel=DEBUG\n", "LogTarget=stderr\n" } },
          -EINVAL,
          { { NULL, NULL }, { NULL, NULL }, { NULL, NULL } } },
        { "with invalid value in custom config file",
          { "NodeName = laptop\n", "NodeName 0 pi\n", { "LogLevel=DEBUG\n", "LogTarget=stderr\n" } },
          -EINVAL,
          { { "NodeName", "laptop" }, { NULL, NULL }, { NULL, NULL } } },
        { "with invalid value in one of the confd files",
          { "NodeName = laptop\n", "NodeName = pi\n", { "LogLevel=DEBUG\n", "LogTarget0stderr\n" } },
          -EINVAL,
          { { "NodeName", "pi" }, { "LogLevel", "DEBUG" }, { NULL, NULL } } },
};

void create_file(const char *file_path, const char *cfg_file_content) {
        FILE *cfg_file = fopen(file_path, "w");
        assert(cfg_file != NULL);
        fputs(cfg_file_content, cfg_file);
        fclose(cfg_file);
}

int setup_test_directory(
                cfg_test_input *test_input,
                char **ret_test_dir_path,
                char **ret_default_config_file,
                char **ret_custom_config_file,
                char **ret_custom_config_directory,
                char *ret_custom_configs[]) {
        int r = asprintf(ret_test_dir_path, "/tmp/cfg-test-%6d", rand() % 999999);
        if (r < 0) {
                return r;
        }

        r = mkdir(*ret_test_dir_path, S_IRUSR | S_IWUSR | S_IXUSR);
        if (r < 0) {
                return r;
        }

        if (test_input->default_config_content != NULL) {
                r = asprintf(ret_default_config_file, "%s/cfg-file-%6d", *ret_test_dir_path, rand() % 999999);
                if (r < 0) {
                        return r;
                }
                create_file(*ret_default_config_file, test_input->default_config_content);
        }
        if (test_input->custom_config_content != NULL) {
                r = asprintf(ret_custom_config_file, "%s/cfg-file-%6d", *ret_test_dir_path, rand() % 999999);
                if (r < 0) {
                        return r;
                }
                create_file(*ret_custom_config_file, test_input->custom_config_content);
        }

        r = asprintf(ret_custom_config_directory, "%s/conf.d", *ret_test_dir_path);
        if (r < 0) {
                return r;
        }

        r = mkdir(*ret_custom_config_directory, S_IRUSR | S_IWUSR | S_IXUSR);
        if (r < 0) {
                return r;
        }

        if (test_input->custom_configs[0] != NULL) {
                r = asprintf(&ret_custom_configs[0],
                             "%s/cfg-file-1-%6d.conf",
                             *ret_custom_config_directory,
                             rand() % 999999);
                if (r < 0) {
                        return r;
                }
                create_file(ret_custom_configs[0], test_input->custom_configs[0]);
        }
        if (test_input->custom_configs[1] != NULL) {
                r = asprintf(&ret_custom_configs[1],
                             "%s/cfg-file-2-%6d.conf",
                             *ret_custom_config_directory,
                             rand() % 999999);
                if (r < 0) {
                        return r;
                }
                create_file(ret_custom_configs[1], test_input->custom_configs[1]);
        }

        return 0;
}

void remove_test_directory(
                char *test_dir_path,
                char *default_config_file,
                char *custom_config_file,
                char *custom_config_directory,
                char *custom_configs[]) {

        if (default_config_file && access(default_config_file, R_OK) == 0) {
                unlink(default_config_file);
        }
        if (custom_config_file && access(custom_config_file, R_OK) == 0) {
                unlink(custom_config_file);
        }
        if (custom_configs[0] && access(custom_configs[0], R_OK) == 0) {
                unlink(custom_configs[0]);
                free(custom_configs[0]);
        }
        if (custom_configs[1] && access(custom_configs[1], R_OK) == 0) {
                unlink(custom_configs[1]);
                free(custom_configs[1]);
        }

        if (custom_config_directory) {
                rmdir(custom_config_directory);
        }
        if (test_dir_path) {
                rmdir(test_dir_path);
        }
}

bool test_cfg_load_complete_configuration(cfg_test_param *test_case) {

        _cleanup_free_ char *test_directory = NULL;
        _cleanup_free_ char *default_config_file = NULL;
        _cleanup_free_ char *custom_config_file = NULL;
        _cleanup_free_ char *custom_config_directory = NULL;
        char *custom_configs[MAX_CUSTOM_CONFIGS] = { NULL, NULL };

        int r = setup_test_directory(
                        &test_case->input,
                        &test_directory,
                        &default_config_file,
                        &custom_config_file,
                        &custom_config_directory,
                        custom_configs);
        if (r < 0) {
                fprintf(stderr,
                        "Failed to setup test directory for test case '%s': %s",
                        test_case->test_case_name,
                        strerror(-r));
                return false;
        }

        struct config *cfg;
        cfg_initialize(&cfg);

        bool result = true;
        r = cfg_load_complete_configuration(
                        cfg, default_config_file, custom_config_file, custom_config_directory);
        if (r != test_case->expected_ret) {
                fprintf(stderr,
                        "Failed '%s': Expected return code '%d', but got '%d'\n",
                        test_case->test_case_name,
                        test_case->expected_ret,
                        r);
                result = false;
        }
        for (unsigned int i = 0; i < MAX_EXPECTED_ENTRIES; i++) {
                cfg_test_expected_entry entry = test_case->expected_entries[i];
                if (entry.key != NULL) {
                        const char *actual = cfg_get_value(cfg, entry.key);
                        if (actual == NULL) {
                                fprintf(stderr,
                                        "Failed '%s': Expected (%s, %s) to be in config, but got (%s, null)\n",
                                        test_case->test_case_name,
                                        entry.key,
                                        entry.value,
                                        entry.key);
                                result = false;
                        } else if (!streq(entry.value, actual)) {
                                fprintf(stderr,
                                        "Failed '%s': Expected (%s, %s) to be in config, but got (%s, %s)\n",
                                        test_case->test_case_name,
                                        entry.key,
                                        entry.value,
                                        entry.key,
                                        actual);
                                result = false;
                        }
                }
        }

        cfg_dispose(cfg);
        remove_test_directory(
                        test_directory,
                        default_config_file,
                        custom_config_file,
                        custom_config_directory,
                        custom_configs);

        return result;
}

int main() {
        bool result = true;

        for (unsigned int i = 0; i < sizeof(test_data) / sizeof(cfg_test_param); i++) {
                result = result && test_cfg_load_complete_configuration(&test_data[i]);
        }

        if (result) {
                return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
}
