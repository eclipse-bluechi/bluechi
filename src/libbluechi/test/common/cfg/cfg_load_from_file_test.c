/* SPDX-License-Identifier: LGPL-2.1-or-later */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libbluechi/common/cfg.h"
#include "libbluechi/common/common.h"

void create_tmp_file(const char *cfg_file_content, char *ret_file_path) {
        sprintf(ret_file_path, "/tmp/cfg-file-%6d", rand() % 999999);
        FILE *cfg_file = fopen(ret_file_path, "w");
        assert(cfg_file != NULL);
        fputs(cfg_file_content, cfg_file);
        fclose(cfg_file);
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
        create_tmp_file(cfg_file_content, cfg_file_name);

        struct config *config = NULL;
        cfg_initialize(&config);

        int result = cfg_load_from_file(config, cfg_file_name);
        assert(result == 0);

        assert(cfg_get_value(config, "key1") != NULL);
        assert(streq(cfg_get_value(config, "key1"), "value1"));
        assert(cfg_get_value(config, "key2") != NULL);
        assert(streq(cfg_get_value(config, "key2"), "value 2"));
        assert(cfg_get_value(config, "key3") == NULL);
        assert(cfg_get_value(config, "key4") != NULL);
        assert(streq(cfg_get_value(config, "key4"), "value 4"));
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
        create_tmp_file(cfg_file_content, cfg_file_name);

        struct config *config = NULL;
        cfg_initialize(&config);

        int result = cfg_load_from_file(config, cfg_file_name);
        assert(result == 0);

        // key value from the global section
        assert(cfg_get_value(config, "key") != NULL);
        assert(streq(cfg_get_value(config, "key"), "value1"));

        // key value from section2
        assert(cfg_s_get_value(config, "section2", "key") != NULL);
        assert(streq(cfg_s_get_value(config, "section2", "key"), "value 2"));

        // key value from section 3
        assert(cfg_s_get_value(config, "section 3", "key") != NULL);
        assert(streq(cfg_s_get_value(config, "section 3", "key"), "value 3"));

        cfg_dispose(config);
        unlink(cfg_file_name);
}

void test_parse_multiline_config_from_file() {
        const char *cfg_file_content =
                        "key1 = value1\n"
                        "key2 = value 2 which is\n"
                        "   distributed on multiple\n"
                        "   lines\n"
                        "key3 = value3\n";

        char cfg_file_name[30];
        create_tmp_file(cfg_file_content, cfg_file_name);

        struct config *config = NULL;
        cfg_initialize(&config);

        int result = cfg_load_from_file(config, cfg_file_name);
        assert(result == 0);

        assert(cfg_get_value(config, "key1") != NULL);
        assert(streq(cfg_get_value(config, "key1"), "value1"));
        assert(cfg_get_value(config, "key2") != NULL);
        assert(streq(cfg_get_value(config, "key2"), "value 2 which isdistributed on multiplelines"));
        assert(cfg_get_value(config, "key3") != NULL);
        assert(streq(cfg_get_value(config, "key3"), "value3"));

        cfg_dispose(config);
        unlink(cfg_file_name);
}

void test_parse_multifile_config_from_file() {
        const char *cfg_file_content_one =
                        "key1 = value 1\n"
                        "key2 = value 2\n"
                        "key3 = value 3\n";

        char cfg_file_name_one[35];
        create_tmp_file(cfg_file_content_one, cfg_file_name_one);

        struct config *config = NULL;
        cfg_initialize(&config);

        int result = cfg_load_from_file(config, cfg_file_name_one);
        unlink(cfg_file_name_one);
        assert(result == 0);

        assert(cfg_get_value(config, "key1") != NULL);
        assert(streq(cfg_get_value(config, "key1"), "value 1"));
        assert(cfg_get_value(config, "key2") != NULL);
        assert(streq(cfg_get_value(config, "key2"), "value 2"));
        assert(cfg_get_value(config, "key3") != NULL);
        assert(streq(cfg_get_value(config, "key3"), "value 3"));

        const char *cfg_file_content_two =
                        "key2 = overridden value\n"
                        "key4 = value 4\n"
                        "key1 = another overridden value\n";

        char cfg_file_name_two[35];
        create_tmp_file(cfg_file_content_two, cfg_file_name_two);

        result = cfg_load_from_file(config, cfg_file_name_two);
        unlink(cfg_file_name_two);
        assert(result == 0);

        assert(cfg_get_value(config, "key1") != NULL);
        assert(streq(cfg_get_value(config, "key1"), "another overridden value"));
        assert(cfg_get_value(config, "key2") != NULL);
        assert(streq(cfg_get_value(config, "key2"), "overridden value"));
        assert(cfg_get_value(config, "key3") != NULL);
        assert(streq(cfg_get_value(config, "key3"), "value 3"));
        assert(cfg_get_value(config, "key4") != NULL);
        assert(streq(cfg_get_value(config, "key4"), "value 4"));

        cfg_dispose(config);
}


int main() {
        test_parse_config_from_file();
        test_parse_sectioned_config_from_file();
        test_parse_multiline_config_from_file();
        test_parse_multifile_config_from_file();

        return EXIT_SUCCESS;
}
