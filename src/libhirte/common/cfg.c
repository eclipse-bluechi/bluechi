/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <asm-generic/errno-base.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libhirte/hashmap/hashmap.h"
#include "libhirte/ini/ini.h"

#include "cfg.h"
#include "common.h"

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
        free(option->section);
        free(option->name);
        free(option->value);
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
/*
 * Handle used to store option parsed from a file into a configuration.
 *
 * Returns 1 on success, 0 on error as per ini library requirements.
 */
static int parsing_handler(void *user, const char *section, const char *name, const char *value) { // NOLINT
        if (user == NULL) {
                return 0;
        }
        struct config *config = (struct config *) user;
        if (cfg_s_set_value(config, section, name, value) != 0) {
                return 0;
        }
        return 1;
}
#pragma GCC diagnostic pop

int cfg_initialize(struct config **config) {
        struct config *new_cfg = malloc(sizeof(struct config));
        if (new_cfg == NULL) {
                // error during configuration structure initialization
                return -ENOMEM;
        }
        new_cfg->cfg_store = hashmap_new(
                        sizeof(struct config_option), 0, 0, 0, option_hash, option_compare, option_destroy, NULL);
        if (new_cfg->cfg_store == NULL) {
                // error during hashmap initialization
                free(new_cfg);
                return -ENOMEM;
        }
        char *section_copy = strdup(CFG_SECT_GLOBAL);
        if (section_copy == NULL) {
                // error during default section initialization
                hashmap_free(new_cfg->cfg_store);
                free(new_cfg);
                return -ENOMEM;
        }
        new_cfg->default_section = section_copy;

        *config = new_cfg;
        return 0;
}

void cfg_dispose(struct config *config) {
        hashmap_free(config->cfg_store);
        free(config->default_section);
        free(config);
}

int cfg_load_complete_configuration(
                struct config *config,
                const char *default_config_file,
                const char *custom_config_file,
                const char *custom_config_directory) {
        int result = 0;

        if (default_config_file != NULL) {
                result = cfg_load_from_file(config, default_config_file);
                if (result != 0) {
                        return result;
                }
        }

        if (custom_config_file != NULL) {
                result = cfg_load_from_file(config, custom_config_file);
                // if custom config file doesn't exist, continue
                if (result != 0 && result != -ENOENT) {
                        return result;
                }
        }

        if (custom_config_directory != NULL) {
                // TODO: https://github.com/containers/hirte/issues/148
        }

        result = cfg_load_from_env(config);

        return result;
}

int cfg_load_from_file(struct config *config, const char *config_file) {
        if (config_file == NULL) {
                return -EINVAL;
        }
        if (access(config_file, F_OK) != 0) {
                return -ENOENT;
        }
        return ini_parse(config_file, parsing_handler, config);
}

int cfg_load_from_env(struct config *config) {
        // env_vars and option_names need to match!
        const char *env_vars[] = { "HIRTE_LOG_LEVEL", "HIRTE_LOG_TARGET", "HIRTE_LOG_IS_QUIET" };
        const char *option_names[] = { CFG_LOG_LEVEL, CFG_LOG_TARGET, CFG_LOG_IS_QUIET };

        int length = sizeof(env_vars) / sizeof(env_vars[0]);
        for (int i = 0; i < length; i++) {
                char *value = getenv(env_vars[i]);
                if (value != NULL && strlen(value) > 0) {
                        // environment variable is defined and has a non-empty value, so store it to the configuration
                        int result = cfg_set_value(config, option_names[i], value);
                        if (result != 0) {
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
        free(config->default_section);
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
                // storing non NULL value, so copy is needed
                value_copy = strdup(option_value);
                if (value_copy == NULL) {
                        free(section_copy);
                        free(name_copy);
                        return -ENOMEM;
                }
        }
        struct config_option *replaced = hashmap_set(
                        config->cfg_store,
                        &(struct config_option){
                                        .section = section_copy, .name = name_copy, .value = value_copy });
        if (hashmap_oom(config->cfg_store)) {
                // OOM during hashmap set operation
                free(section_copy);
                free(name_copy);
                free(value_copy);
                return -ENOMEM;
        }

        if (replaced != NULL) {
                // option has been replaced with the new one, we need to deallocate the old one
                free(replaced->section);
                free(replaced->name);
                free(replaced->value);
        }
        return 0;
}

const char *cfg_get_value(struct config *config, const char *option_name) {
        return cfg_s_get_value(config, config->default_section, option_name);
}

const char *cfg_s_get_value(struct config *config, const char *section_name, const char *option_name) {
        struct config_option *found = (struct config_option *) hashmap_get(
                        config->cfg_store,
                        &(struct config_option){ .section = (char *) section_name,
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

void config_iterate(struct config *config, bool (*iter)(const void *item, void *udata), void *udata) {
        hashmap_scan(config->cfg_store, iter, udata);
}
