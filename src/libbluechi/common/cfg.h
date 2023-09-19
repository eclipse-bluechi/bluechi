/* SPDX-License-Identifier: LGPL-2.1-or-later */

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
 * Configuration section for bluechi manager.
 */
#define CFG_SECT_BLUECHI "bluechi-controller"

/*
 * Configuration section for bluechi agent.
 */
#define CFG_SECT_AGENT "bluechi-agent"

/*
 * Structure holding the configuration
 */
typedef struct config config;

/* Standard configuration file locations
 */
#define CFG_ETC_BC_CONF CONFIG_H_SYSCONFDIR "/bluechi/controller.conf"
#define CFG_ETC_BC_AGENT_CONF CONFIG_H_SYSCONFDIR "/bluechi/agent.conf"
#define CFG_AGENT_DEFAULT_CONFIG CONFIG_H_DATADIR "/bluechi-agent/config/agent.conf"
#define CFG_BC_DEFAULT_CONFIG CONFIG_H_DATADIR "/bluechi/config/controller.conf"
#define CFG_ETC_AGENT_CONF_DIR CONFIG_H_SYSCONFDIR "/bluechi/agent.conf.d"
#define CFG_ETC_BC_CONF_DIR CONFIG_H_SYSCONFDIR "/bluechi/controller.conf.d"

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
 * Iterates over all entries in src and copies the key=value pairs
 * into the dst config. If a key from src already exists in dst, the
 * value is overridden.
 *
 * Returns 0 if successful, otherwise forwards error from cfg_s_set_value.
 */
int cfg_copy_overwrite(struct config *src, struct config *dst);

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
 * Returns 0 if successful. If a parsing error occurred, a positive integer indicating the line of error is
 * returned. On all other errors a standard error code will be returned.
 */
int cfg_load_from_file(struct config *config, const char *config_file);


/*
 * Load the application configuration from files inside the specified directory and override any existing options values.
 *
 * Returns 0 if successful, otherwise standard error code.
 */
int cfg_load_from_dir(struct config *config, const char *custom_config_directory);

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
 * Dump all key-value pairs in the config to a string.
 *
 * Returns a string containing all key-value pairs in the configuration separated by new lines.
 * The caller is responsible for freeing the returned string.
 */
const char *cfg_dump(struct config *config);

/*
 * Populate the default agent configuration from the source code
 */
int cfg_agent_def_conf(struct config *config);

/*
 * Populate the default manager configuration from the source code
 */
int cfg_manager_def_conf(struct config *config);
