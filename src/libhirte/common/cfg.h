/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include <stdbool.h>


/*
 * Existing configuration file options
 */
#define CFG_LOG_LEVEL "LogLevel"
#define CFG_LOG_TARGET "LogTarget"
#define CFG_LOG_IS_QUIET "LogIsQuiet"
#define CFG_MANAGER_HOST "ManagerHost"
#define CFG_MANAGER_PORT "ManagerPort"
#define CFG_MANAGER_ADDRESS "ManagerAddress"
#define CFG_NODE_NAME "NodeName"
#define CFG_ALLOWED_NODE_NAMES "AllowedNodeNames"
#define CFG_HEARTBEAT_INTERVAL "HeartbeatInterval"

/*
 * Global section - this is used, when configuration options are specified in the configuration file
 * without any section
 */
#define CFG_SECT_GLOBAL ""

/*
 * Configuration section for hirte manager.
 */
#define CFG_SECT_HIRTE "hirte"

/*
 * Configuration section for hirte agent.
 */
#define CFG_SECT_AGENT "hirte-agent"

/*
 * Structure holding the configuration
 */
typedef struct config config;

/* Standard configuration file locations
 */
#define CFG_ETC_HIRTE_CONF CONFIG_H_SYSCONFDIR "/hirte/hirte.conf"
#define CFG_ETC_HIRTE_AGENT_CONF CONFIG_H_SYSCONFDIR "/hirte/agent.conf"
#define CFG_AGENT_DEFAULT_CONFIG CONFIG_H_DATADIR "/hirte-agent/config/hirte-default.conf"
#define CFG_HIRTE_DEFAULT_CONFIG CONFIG_H_DATADIR "/hirte/config/hirte-default.conf"

/*
 * An item in a configuration map
 */
struct config_option {
        char *section;
        char *name;
        char *value;
};

/*
 * Initialize the application configuration object.
 *
 * Returns 0 if successful, otherwise standard error code
 */
int cfg_initialize(struct config **config);

/*
 * Dispose the application configuration object
 */
void cfg_dispose(struct config *config);

/*
 * Load the application configuration using standardized flow:
 *
 *   1. Load default configuration file
 *   2. Load custom application configuration file, for example /etc/<NAME>.conf
 *   3. Load custom application configuration directory, for example /etc/<NAME>.conf.d
 *   4. Load configuration from environment variables
 *
 * WARNING: this method should be invoked only once after execution of config_initialize() to ensure
 * standardized configuration loading flow is used.
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_load_complete_configuration(
                struct config *config,
                const char *default_config_file,
                const char *custom_config_file,
                const char *custom_config_directory);

/*
 * Load the application configuration from the specified file and override any existing options values.
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_load_from_file(struct config *config, const char *config_file);

/*
 * Load the application configuration from the predefined environment variables and override any existing
 * options values.
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_load_from_env(struct config *config);

/*
 * Returns the default section.
 *
 * WARNING: the default section value is returned directly, DO NOT modify it!!!
 */
const char *cfg_get_default_section(struct config *config);

/*
 * Sets the default section, which will be used to set/get values without explicit section specification.
 * If this method is not called, then empty string is used as the default section (empty string is used by
 * inih parser for values which are specified without any section in configuration file).
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_set_default_section(struct config *config, const char *section_name);

/*
 * Set the value of the specified configuration option in the default section.
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_set_value(struct config *config, const char *option_name, const char *option_value);

/*
 * Set the value of the specified configuration option in the specified section.
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_s_set_value(
                struct config *config,
                const char *section_name,
                const char *option_name,
                const char *option_value);

/*
 * Get value of the specified configuration option in the default section.
 *
 * Returns the value of the specified option or NULL if option doesn't exist or it doesn't have a value.
 */
const char *cfg_get_value(struct config *config, const char *option_name);

/*
 * Get value of the specified configuration option in the specified section.
 *
 * Returns the value of the specified option or NULL if option doesn't exist or it doesn't have a value.
 */
const char *cfg_s_get_value(struct config *config, const char *section, const char *option_name);

/*
 * Get value of the specific configuration option in the default section and convert it to bool type.
 *
 * Returns bool value after successful conversion from string or false, if there was an error during
 * conversion or if the option doesn't exist or have a NULL value.
 */
bool cfg_get_bool_value(struct config *config, const char *option_name);

/*
 * Get value of the specific configuration option in the specified section and convert it to bool type.
 *
 * Returns bool value after successful conversion from string or false, if there was an error during
 * conversion or if the option doesn't exist or have a NULL value.
 */
bool cfg_s_get_bool_value(struct config *config, const char *section, const char *option_name);

/*
 * Iterate over configuration and for each item execute the iter function.
 * For example using following iterator method would print the whole configuration:
 *
 *     bool config_option_iterator(const void *item, void *udata) {
 *         const struct config_option *option = item;
 *         printf("%s = %s\n", option->name, option->value);
 *         return true;
 *     }
 *
 * WARNING: configuration items are accessed directly, DO NOT modify them during iteration!!!
 */
void cfg_iterate(struct config *config, bool (*iter)(const void *item, void *udata), void *udata);
