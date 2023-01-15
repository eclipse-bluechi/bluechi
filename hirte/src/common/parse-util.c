#include "parse-util.h"
#include "../common/strings.h"
#include "../ini/config.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool parse_long(const char *in, long *ret) {
        const int base = 10;
        char *x = NULL;

        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        errno = 0;
        *ret = strtol(in, &x, base);

        if (errno > 0 || !x || x == in || *x != 0) {
                return false;
        }

        return true;
}

bool parse_port(const char *in, uint16_t *ret) {
        if (in == NULL || strlen(in) == 0) {
                return false;
        }

        bool r = false;
        long l = 0;
        r = parse_long(in, &l);

        if (!r || l < 0 || l > USHRT_MAX) {
                return false;
        }

        *ret = (uint16_t) l;
        return true;
}

bool is_IP(char *str) {
        long num = 0;
        int dots = 0;
        char *ptr = NULL;

        if (str == NULL) {
                return false;
        }

        ptr = str;
        // NOLINTNEXTLINE
        while (*ptr != '\0') {
                if (*ptr == '.') {
                        dots++;
                } else if (!isdigit(*ptr)) {
                        return false;
                }
                ptr++;
        }

        if (dots != 3) {
                return false;
        }
        int const max = 255;
        ptr = strtok(str, ".");
        int const base = 10;
        while (ptr != NULL) {
                num = strtol(ptr, NULL, base);
                if (num >= 0 && num <= max) {
                        ptr = strtok(NULL, ".");
                } else {
                        return false;
                }
        }

        return true;
}

bool is_port(char *str) {
        long port = 0;
        int const max_port = 65535;

        if (str == NULL) {
                return 0;
        }
        int const base = 10;
        port = strtol(str, NULL, base);
        if (port < 0 || port > max_port) {
                return 0;
        }
        return 1;
}

bool is_IPs(char *str) {
        int const max_number_of_digit_ip = 16;

        char *ip = (char *) malloc(max_number_of_digit_ip);

        char *ptr = str;
        char *tmp = NULL;
        while (*ptr) {
                tmp = ip;
                while (*ptr != ',' && *ptr != '\0') {
                        *tmp = *ptr;
                        ptr++;
                        tmp++;
                }
                int temp = is_IP(ip);
                if (!temp) {
                        free(ip);
                        return false;
                }
                if (*ptr == ',') {
                        ptr++;
                        while (*ptr == ' ' && *ptr) {
                                ptr++;
                        }
                }
        }
        free(ip);
        return true;
}


int validate_topic_key_value(char *topic, char *key, char *value) {
        if (streq("Node", topic)) {
                if (streq("OrchestratorIP", key)) {
                        return is_IP(value);
                }
                if (streq("OrchestratorPort", key)) {
                        return is_port(value);
                }
                return 0;
        }
        if (streq("Orchestrator", topic)) {
                if (streq("Port", key)) {
                        return is_port(value);
                }
                if (streq("ExpectedNodes", key)) {
                        return is_IPs(value);
                }
                return 0;
        }
        return 1;
}

/*This function will get a hashmap from parsing_ini_file function and validate  */
bool validate_hashmap(struct hashmap *hm) {
        size_t iter = 0;
        void *item = NULL;
        while (hashmap_iter(hm, &iter, &item)) {
                topic *tp = item;
                size_t iter_kv = 0;
                void *item_kv = NULL;
                while (hashmap_iter(tp->keys_and_values, &iter_kv, &item_kv)) {
                        keyValue *kv = (keyValue *) item_kv;
                        int temp = validate_topic_key_value(tp->topic, kv->key, kv->value);
                        if (!(temp)) {
                                return false;
                        }
                }
        }
        return true;
}
