#include <assert.h>
#include <stdlib.h>

#include "libhirte/common/common.h"

#include "config.h"
#include "ini.h"

typedef struct {
        char *key;
        char *value;
} keyValue;

struct topic {
        char *topic;
        struct hashmap *keys_and_values;
};

static void key_value_free(void *item) {
        keyValue *kv = item;
        free(kv->key);
        free(kv->value);
}

static void topic_free(void *item) {
        topic *topic = item;
        free(topic->topic);
        hashmap_free(topic->keys_and_values);
}

int key_value_compare_key(const void *a, const void *b, UNUSED void *udata) {
        const keyValue *kva = a;
        const keyValue *kvb = b;
        return strcmp(kva->key, kvb->key);
}

uint64_t key_value_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const keyValue *kv = item;
        return hashmap_sip(kv->key, strlen(kv->key), seed0, seed1);
}

int topic_compare_topic_name(const void *a, const void *b, UNUSED void *udata) {
        const topic *ta = a;
        const topic *tb = b;
        return strcmp(ta->topic, tb->topic);
}

uint64_t topic_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        const topic *top = item;
        return hashmap_sip(top->topic, strlen(top->topic), seed0, seed1);
}

struct topic *config_lookup_topic(config *config, const char *name) {
        return hashmap_get(config, &(topic){ .topic = (char *) name });
}

struct topic *config_ensure_topic(config *config, const char *name) {
        struct topic *old = config_lookup_topic(config, name);
        if (old != NULL) {
                return old;
        }

        char *sec = strdup(name);
        if (sec == NULL) {
                return NULL;
        }

        struct hashmap *hm = hashmap_new(
                        sizeof(keyValue), 0, 0, 0, key_value_hash, key_value_compare_key, key_value_free, NULL);
        if (hm == NULL) {
                free(sec);
                return NULL;
        }

        topic newtopic = { sec, hm };
        hashmap_set(config, &newtopic);
        if (hashmap_oom(config)) {
                topic_free(&newtopic);
                return NULL;
        }

        return config_lookup_topic(config, name);
}

const char *topic_lookup(topic *t, const char *key) {
        keyValue *kv = hashmap_get(t->keys_and_values, &(keyValue){ .key = (char *) key });
        if (kv == NULL) {
                return NULL;
        }
        return kv->value;
}

bool topic_set(topic *t, const char *key, const char *value) {
        char *key_dup = strdup(key);
        if (key_dup == NULL) {
                return false;
        }
        char *value_dup = strdup(value);
        if (value_dup == NULL) {
                free(key_dup);
                return false;
        }

        keyValue kv = { key_dup, value_dup };
        keyValue *replaced = hashmap_set(t->keys_and_values, &kv);
        if (hashmap_oom(t->keys_and_values)) {
                key_value_free(&kv);
                return false;
        }
        if (replaced) {
                key_value_free(replaced);
        }
        return true;
}

const char *config_lookup(config *config, const char *topicname, const char *key) {
        topic *t = config_lookup_topic(config, topicname);
        if (t == NULL) {
                return NULL;
        }
        return topic_lookup(t, key);
}

/*
 * This function will be used in ini_parse() for node ini file parsing.
 */
static int handler(void *user, const char *section, const char *name, const char *value) {
        config *c = (config *) user;

        assert(section != NULL);
        assert(name != NULL);
        assert(user != NULL);
        assert(value != NULL);

        topic *t = config_ensure_topic(c, section);
        if (t == NULL) {
                return 1; /* out of memory */
        }

        if (!topic_set(t, name, value)) {
                return 1;
        }

        return 0;
}

config *new_config(void) {
        return hashmap_new(sizeof(topic), 0, 0, 0, topic_hash, topic_compare_topic_name, topic_free, NULL);
}

/* Get the file path
    if it can't find the or cant parse the path it will return configurationOrch
    with nulls else will return configurationOrch with all the data */
config *parsing_ini_file(const char *file) {
        config *config = new_config();
        if (config == NULL) {
                return NULL; /* oom */
        }
        if (file == NULL) {
                return config;
        }

        if (ini_parse(file, handler, config) < 0) {
                printf("Can't load the file %s\n", file);
                return config;
        }
        return config;
}

void print_all_topics(config *config) {
        if (config != NULL) {
                return;
        }
        size_t iter = 0;
        void *item = NULL;
        while (hashmap_iter(config, &iter, &item)) {
                topic *tp = item;
                size_t iter_kv = 0;
                void *item_kv = NULL;
                printf("[%s]\n", tp->topic);
                while (hashmap_iter(tp->keys_and_values, &iter_kv, &item_kv)) {
                        keyValue *kv = item_kv;
                        printf("\t%s :", kv->key);
                        printf(" %s\n", kv->value);
                }
        }
}

void free_hashmapp(struct hashmap **hmp) {
        if (hmp && *hmp) {
                hashmap_free(*hmp);
        }
}

void free_config(config *config) {
        hashmap_free(config);
}

void free_configp(config **configp) {
        if (configp && *configp) {
                free_config(*configp);
        }
}
