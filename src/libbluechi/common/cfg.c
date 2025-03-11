/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <asm-generic/errno-base.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <hashmap.h>
#include <ini.h>

#include "libbluechi/common/list.h"
#include "libbluechi/common/network.h"

#include "cfg.h"
#include "common.h"

static inline void hashmap_freep(void *pmap) {
        struct hashmap *map = *(struct hashmap **) pmap;
        hashmap_free(map);
}

#define _cleanup_hashmap_ _cleanup_(hashmap_freep)

/*
 * Structure holding the configuration is just hiding that it uses a hashmap structure internally
 */
struct config {
        struct hashmap *cfg_store;
        char *default_section;
};

static int option_compare(const void *a, const void *b, UNUSED void *udata) {
        const struct config_option *option_a = a;
        const struct config_option *option_b = b;
        return !(streq(option_a->section, option_b->section) && streq(option_a->name, option_b->name));
}

static void option_destroy(void *item) {
        struct config_option *option = item;
        free_and_null(option->section);
        free_and_null(option->name);
        free_and_null(option->value);
}

static uint64_t option_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const struct config_option *option_a = item;
        char *sect_and_name = NULL;
        // there is no way how to tell hashmap implementation that there was an error during hash generation
        assert(asprintf(&sect_and_name, "%s=%s", option_a->section, option_a->name) != -1);
        uint64_t hash = hashmap_sip(sect_and_name, strlen(sect_and_name), seed0, seed1);
        free(sect_and_name);
        return hash;
}

/*
 * Handle used to store option parsed from a file into a configuration.
 *
 * Returns 1 on success, 0 on error as per ini library requirements.
 */
static int parsing_handler(void *user, const char *section, const char *name, const char *value) {
        if (user == NULL) {
                return 0;
        }

        struct config *config = (struct config *) user;

        const char *existing_value = cfg_s_get_value(config, section, name);
        if (existing_value != NULL) {
                _cleanup_free_ const char *new_value = strcat_dup(existing_value, value);
                if (cfg_s_set_value(config, section, name, new_value) != 0) {
                        return 0;
                }
                return 1;
        }
        if (cfg_s_set_value(config, section, name, value) != 0) {
                return 0;
        }

        return 1;
}

int cfg_initialize(struct config **config) {
        struct config *new_cfg = malloc(sizeof(struct config));
        if (new_cfg == NULL) {
                return -ENOMEM;
        }
        new_cfg->cfg_store = hashmap_new(
                        sizeof(struct config_option), 0, 0, 0, option_hash, option_compare, option_destroy, NULL);
        if (new_cfg->cfg_store == NULL) {
                free(new_cfg);
                return -ENOMEM;
        }
        char *section_copy = strdup(CFG_SECT_GLOBAL);
        if (section_copy == NULL) {
                hashmap_free(new_cfg->cfg_store);
                free(new_cfg);
                return -ENOMEM;
        }
        new_cfg->default_section = section_copy;

        *config = new_cfg;
        return 0;
}

int cfg_copy_overwrite(struct config *src, struct config *dst) {
        int r = 0;
        void *item = NULL;
        size_t i = 0;
        while (hashmap_iter(src->cfg_store, &i, &item)) {
                struct config_option *opt = item;
                r = cfg_s_set_value(dst, opt->section, opt->name, opt->value);
                if (r < 0) {
                        return r;
                }
        }
        return 0;
}

void cfg_dispose(struct config *config) {
        hashmap_free(config->cfg_store);
        free_and_null(config->default_section);
        free_and_null(config);
}

int cfg_load_complete_configuration(
                struct config *config,
                const char *default_config_file,
                const char *custom_config_file,
                const char *custom_config_directory,
                const char *cli_option_config_file) {
        int result = 0;

        /* 1. Load default configuration file */
        if (default_config_file != NULL) {
                result = cfg_load_from_file(config, default_config_file);
                if (result < 0) {
                        fprintf(stderr,
                                "Error loading default configuration file '%s': '%s'.\n",
                                default_config_file,
                                strerror(-result));
                        return result;
                } else if (result > 0) {
                        fprintf(stderr,
                                "Error parsing configuration file '%s' on line %d\n",
                                default_config_file,
                                result);
                        return -EINVAL;
                }
        }

        /* 2. Load custom application configuration file, for example /etc/<NAME>.conf */
        if (custom_config_file != NULL) {
                result = cfg_load_from_file(config, custom_config_file);
                if (result != 0 && result != -ENOENT) {
                        if (result < 0) {
                                fprintf(stderr,
                                        "Error loading custom configuration file '%s': '%s'.\n",
                                        custom_config_file,
                                        strerror(-result));
                                return result;
                        } else if (result > 0) {
                                fprintf(stderr,
                                        "Error parsing configuration file '%s' on line %d\n",
                                        custom_config_file,
                                        result);
                                return -EINVAL;
                        }
                }
        }

        /* 3. Load custom application configuration directory, for example /etc/<NAME>.conf.d */
        if (custom_config_directory != NULL) {
                result = cfg_load_from_dir(config, custom_config_directory);
                if (result < 0) {
                        fprintf(stderr,
                                "Error loading configuration from conf.d dir '%s': '%s'.\n",
                                custom_config_directory,
                                strerror(-result));
                        return result;
                } else if (result > 0) {
                        /* error already logged in cfg_load_from_dir */
                        return -EINVAL;
                }
        }

        /* 4. Load configuration from environment variables */
        result = cfg_load_from_env(config);

        /* 5. Load custom application configuration file from CLI option parameter */
        if (cli_option_config_file != NULL) {
                result = cfg_load_from_file(config, cli_option_config_file);
                if (result < 0) {
                        fprintf(stderr,
                                "Error loading configuration file '%s', error code '%s'.\n",
                                cli_option_config_file,
                                strerror(-result));
                        return result;
                } else if (result > 0) {
                        fprintf(stderr,
                                "Error parsing configuration file '%s' on line %d\n",
                                cli_option_config_file,
                                result);
                        return -EINVAL;
                }
        }

        return result;
}

int cfg_load_from_file(struct config *config, const char *config_file) {
        if (config_file == NULL) {
                return -EINVAL;
        }
        if (access(config_file, R_OK) != 0) {
                return -errno;
        }

        struct config *tmp_config = NULL;
        int r = cfg_initialize(&tmp_config);
        if (r < 0) {
                return r;
        }

        r = ini_parse(config_file, parsing_handler, tmp_config);
        if (r != 0) {
                cfg_dispose(tmp_config);
                return r;
        }

        r = cfg_copy_overwrite(tmp_config, config);
        cfg_dispose(tmp_config);
        return r;
}

int is_file_name_ending_with_conf(const struct dirent *entry) {
        return ends_with(entry->d_name, ".conf");
}

int cfg_load_from_dir(struct config *config, const char *custom_config_directory) {
        struct dirent **all_files_in_dir = NULL;
        int number_of_files = 0;

        if (access(custom_config_directory, F_OK) != 0) {
                // consider a missing confd directory as no error
                return 0;
        }
        number_of_files = scandir(
                        custom_config_directory, &all_files_in_dir, &is_file_name_ending_with_conf, alphasort);
        if (number_of_files < 0) {
                return -errno;
        }

        int r = 0;
        /* load each conf.d file and on any error exit loop and clean-up afterwards */
        for (int i = 0; i < number_of_files; i++) {
                char *file_name = all_files_in_dir[i]->d_name;

                _cleanup_free_ char *file = NULL;
                r = asprintf(&file, "%s/%s", custom_config_directory, file_name);
                if (r < 0) {
                        r = -ENOMEM;
                        break;
                }

                r = cfg_load_from_file(config, file);
                if (r < 0) {
                        fprintf(stderr, "Error loading confd file '%s' on line %d.\n", file, r);
                        break;
                }
        }

        /* free allocated memory */
        for (int i = 0; i < number_of_files; i++) {
                free(all_files_in_dir[i]);
        }
        free(all_files_in_dir);

        return r;
}

int cfg_load_from_env(struct config *config) {
        // env_vars and option_names need to match!
        const char *env_vars[] = { "BC_LOG_LEVEL", "BC_LOG_TARGET", "BC_LOG_IS_QUIET" };
        const char *option_names[] = { CFG_LOG_LEVEL, CFG_LOG_TARGET, CFG_LOG_IS_QUIET };

        int length = sizeof(env_vars) / sizeof(env_vars[0]);
        for (int i = 0; i < length; i++) {
                char *value = getenv(env_vars[i]);
                if (value != NULL && strlen(value) > 0) {
                        int result = cfg_set_value(config, option_names[i], value);
                        if (result < 0) {
                                fprintf(stderr,
                                        "Error setting env value for '%s': '%s'.\n",
                                        option_names[i],
                                        strerror(-result));
                                return result;
                        }
                }
        }
        return 0;
}

const char *cfg_get_default_section(struct config *config) {
        return config->default_section;
}

int cfg_set_default_section(struct config *config, const char *section_name) {
        if (config == NULL) {
                return -EINVAL;
        }
        if (section_name == NULL) {
                return -EINVAL;
        }
        char *section_copy = strdup(section_name);
        if (section_copy == NULL) {
                return -ENOMEM;
        }
        free_and_null(config->default_section);
        config->default_section = section_copy;

        return 0;
}

int cfg_set_value(struct config *config, const char *option_name, const char *option_value) {
        return cfg_s_set_value(config, config->default_section, option_name, option_value);
}

int cfg_s_set_value(
                struct config *config,
                const char *section_name,
                const char *option_name,
                const char *option_value) {
        char *section_copy = strdup(section_name);
        if (section_copy == NULL) {
                return -ENOMEM;
        }

        char *name_copy = strdup(option_name);
        if (name_copy == NULL) {
                free(section_copy);
                return -ENOMEM;
        }

        char *value_copy = NULL;
        if (option_value != NULL && strlen(option_value) > 0) {
                value_copy = strdup(option_value);
                if (value_copy == NULL) {
                        free(section_copy);
                        free(name_copy);
                        return -ENOMEM;
                }
        }
        struct config_option *replaced = (struct config_option *) hashmap_set(
                        config->cfg_store,
                        &(struct config_option) {
                                        .section = section_copy, .name = name_copy, .value = value_copy });
        if (hashmap_oom(config->cfg_store)) {
                free(section_copy);
                free(name_copy);
                free(value_copy);
                return -ENOMEM;
        }

        if (replaced != NULL) {
                free_and_null(replaced->section);
                free_and_null(replaced->name);
                free_and_null(replaced->value);
        }
        return 0;
}

const char *cfg_get_value(struct config *config, const char *option_name) {
        return cfg_s_get_value(config, config->default_section, option_name);
}

const char *cfg_s_get_value(struct config *config, const char *section_name, const char *option_name) {
        struct config_option *found = (struct config_option *) hashmap_get(
                        config->cfg_store,
                        &(struct config_option) { .section = (char *) section_name,
                                                  .name = (char *) option_name });
        if (found != NULL) {
                return found->value;
        }

        return NULL;
}

bool cfg_get_bool_value(struct config *config, const char *option_name) {
        return cfg_s_get_bool_value(config, config->default_section, option_name);
}

bool cfg_s_get_bool_value(struct config *config, const char *section_name, const char *option_name) {
        bool result = false;
        const char *value = cfg_s_get_value(config, section_name, option_name);
        if (value != NULL) {
                result = streqi("true", value) || streqi("t", value) || streqi("yes", value) ||
                                streqi("y", value) || streqi("on", value);
        }
        return result;
}

static int strp_compare(const void *a, const void *b, UNUSED void *udata) {
        const char *a_str = *(const char **) a;
        const char *b_str = *(const char **) b;

        return strcmp(a_str, b_str);
}

static uint64_t strp_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const char *str = *(const char **) item;
        return hashmap_sip(str, strlen(str), seed0, seed1);
}

char **cfg_list_sections(struct config *config) {
        _cleanup_hashmap_ struct hashmap *sections = hashmap_new(
                        sizeof(char *), 0, 0, 0, strp_hash, strp_compare, NULL, 0);
        if (sections == NULL) {
                return NULL;
        }

        void *item = NULL;
        size_t i = 0;

        while (hashmap_iter(config->cfg_store, &i, &item)) {
                struct config_option *opt = item;
                if (!hashmap_get(sections, &opt->section)) {
                        hashmap_set(sections, &opt->section);
                        if (hashmap_oom(sections)) {
                                return NULL;
                        }
                }
        }

        size_t n_sections = hashmap_count(sections);
        _cleanup_freev_ char **res = calloc(n_sections + 1, sizeof(char *));
        if (res == NULL) {
                return NULL;
        }

        item = 0;
        i = 0;
        size_t n = 0;
        while (hashmap_iter(sections, &i, &item)) {
                char **section = item;
                char *copy = strdup(*section);
                if (copy == NULL) {
                        return NULL;
                }

                res[n++] = copy;
        }

        qsort_r(res, n, sizeof(char *), strp_compare, NULL);

        return steal_pointer(&res);
}

const char *cfg_dump(struct config *config) {
        if (config == NULL) {
                return NULL;
        }

        _cleanup_freev_ char **sections = cfg_list_sections(config);
        if (sections == NULL) {
                return NULL;
        }

        _cleanup_string_builder_ StringBuilder builder = STRING_BUILDER_INIT;
        if (!string_builder_init(&builder, 0)) {
                return NULL;
        }

        for (size_t s = 0; sections != NULL && sections[s] != NULL; s++) {
                const char *section = sections[s];

                if (s != 0) {
                        string_builder_append(&builder, "\n");
                }

                if (!string_builder_printf(&builder, "[%s]\n", section)) {
                        string_builder_destroy(&builder);
                        return NULL;
                }

                void *item = NULL;
                size_t i = 0;
                while (hashmap_iter(config->cfg_store, &i, &item)) {
                        struct config_option *opt = item;

                        if (!streq(opt->section, section)) {
                                continue;
                        }

                        if (!string_builder_printf(
                                            &builder, "%s=%s\n", opt->name, opt->value ? opt->value : "")) {
                                string_builder_destroy(&builder);
                                return NULL;
                        }
                }
        }
        return steal_pointer(&builder.str);
}

static int cfg_def_conf(struct config *config) {
        int result = 0;

        if ((result = cfg_set_value(config, CFG_LOG_LEVEL, log_level_to_string(LOG_LEVEL_INFO))) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_LOG_TARGET, BC_LOG_TARGET_JOURNALD)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_LOG_IS_QUIET, NULL)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_IP_RECEIVE_ERRORS, BC_DEFAULT_IP_RECEIVE_ERROR)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_TCP_KEEPALIVE_TIME, BC_DEFAULT_TCP_KEEPALIVE_TIME)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_TCP_KEEPALIVE_INTERVAL, BC_DEFAULT_TCP_KEEPALIVE_INTERVAL)) !=
            0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_TCP_KEEPALIVE_COUNT, BC_DEFAULT_TCP_KEEPALIVE_COUNT)) != 0) {
                return result;
        }

        return 0;
}

int cfg_agent_def_conf(struct config *config) {
        int result = cfg_set_default_section(config, CFG_SECT_AGENT);
        if (result != 0) {
                return result;
        }

        if ((result = cfg_def_conf(config)) != 0) {
                return result;
        }

        _cleanup_free_ char *default_node_name = get_hostname();
        if ((result = cfg_set_value(config, CFG_NODE_NAME, default_node_name)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_CONTROLLER_HOST, BC_DEFAULT_HOST)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_HEARTBEAT_INTERVAL, AGENT_DEFAULT_HEARTBEAT_INTERVAL_MSEC)) !=
            0) {
                return result;
        }

        if ((result = cfg_set_value(
                             config,
                             CFG_CONTROLLER_HEARTBEAT_THRESHOLD,
                             AGENT_DEFAULT_CONTROLLER_HEARTBEAT_THRESHOLD_MSEC)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_CONTROLLER_PORT, BC_DEFAULT_PORT)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(
                             config,
                             CFG_CONNECTION_RETRY_COUNT_UNTIL_QUIET,
                             AGENT_DEFAULT_CONNECTION_RETRY_COUNT_UNTIL_QUIET)) != 0) {
                return result;
        }

        return 0;
}

int cfg_controller_def_conf(struct config *config) {
        int result = 0;

        result = cfg_set_default_section(config, CFG_SECT_BLUECHI);
        if (result != 0) {
                return result;
        }

        if ((result = cfg_def_conf(config)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_CONTROLLER_USE_TCP, CONTROLLER_DEFAULT_USE_TCP)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_CONTROLLER_USE_UDS, CONTROLLER_DEFAULT_USE_UDS)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_CONTROLLER_PORT, BC_DEFAULT_PORT)) != 0) {
                return result;
        }

        if ((result = cfg_set_value(config, CFG_ALLOWED_NODE_NAMES, "")) != 0) {
                return result;
        }

        if ((result = cfg_set_value(
                             config, CFG_HEARTBEAT_INTERVAL, CONTROLLER_DEFAULT_HEARTBEAT_INTERVAL_MSEC)) !=
            0) {
                return result;
        }

        if ((result = cfg_set_value(
                             config,
                             CFG_NODE_HEARTBEAT_THRESHOLD,
                             CONTROLLER_DEFAULT_NODE_HEARTBEAT_THRESHOLD_MSEC)) != 0) {
                return result;
        }

        return 0;
}
