#include "../../src/ini/config.h"
#include "../../src/common/hashmap/hashmap.h" 
#include "../../src/common/parse-util.c"
#include <stdlib.h>
#include <stdio.h>

#define assert_se(expr)          \
        do {                     \
                if (!(expr)) {   \
                        exit(1); \
                }                \
        } while (false)

bool test_config() {
    printf("Starting the parse test\n");
    char * config_file = "../../test/ini/example.ini";
    struct hashmap * _cleanup_hashmap_ test_hm = parsing_ini_file(config_file);
    assert_se(test_hm != NULL);
    int tmp = validate_hashmap(test_hm);
    assert_se(tmp != 0);
    print_all_topics(test_hm);
    printf("Done testing and validating ini file parse\n");
    return true;
}

void main() {
    test_config();
}