#include "../common/hashmap/hashmap.h"

typedef struct {
        const char *key;
        const char *value;
} keyValue;

typedef struct {
        const char *topic;
        struct hashmap *keys_and_values;
} topic;


struct hashmap *parsing_ini_file(const char *file);
void free_topics_hashmap(struct hashmap **topics);

void *print_all_topics(struct hashmap *topics);

#define _cleanup_hashmap_ __attribute__((__cleanup__(free_topics_hashmap)))
