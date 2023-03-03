/* SPDX-License-Identifier: GPL-2.0-or-later */
#pragma once

#include "libhirte/hashmap/hashmap.h"

typedef struct hashmap config;
typedef struct topic topic;


config *parsing_ini_file(const char *file);
config *new_config(void);
void free_config(config *config);
void free_configp(config **configp);

void print_all_topics(config *config);
topic *config_lookup_topic(config *config, const char *name);
topic *config_ensure_topic(config *config, const char *name);
const char *config_lookup(config *config, const char *topicname, const char *key);

const char *topic_lookup(topic *t, const char *key);
bool topic_set(topic *t, const char *key, const char *value);

void free_hashmapp(struct hashmap **hmp);

bool is_true_value(const char *value);
bool is_false_value(const char *value);

#define _cleanup_hashmap_ __attribute__((__cleanup__(free_hashmap)))
#define _cleanup_config_ __attribute__((__cleanup__(free_configp)))
