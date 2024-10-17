/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "command.h"

#include "libbluechi/common/parse-util.h"
#include "libbluechi/common/string-util.h"

Command *new_command() {
        _cleanup_command_ Command *command = malloc0(sizeof(Command));
        if (command == NULL) {
                return NULL;
        }

        command->ref_count = 1;
        command->op = NULL;
        command->opargc = 0;
        command->opargv = NULL;
        command->is_help = false;

        LIST_HEAD_INIT(command->command_options);

        return steal_pointer(&command);
}

void command_unref(Command *command) {
        command->ref_count--;
        if (command->ref_count != 0) {
                return;
        }

        CommandOption *option = NULL;
        CommandOption *next_option = NULL;
        LIST_FOREACH_SAFE(options, option, next_option, command->command_options) {
                command_option_free(option);
        }

        free_and_null(command);
}

int command_execute(Command *command, void *userdata) {
        const Method *method = command->method;
        if (command->is_help) {
                method->usage();
                return 0;
        }
        if (command->opargc < method->min_args) {
                fprintf(stderr, "Too few arguments for method %s\n", method->name);
                return -EINVAL;
        }
        if (command->opargc > method->max_args) {
                fprintf(stderr, "Too many arguments for method %s\n", method->name);
                return -EINVAL;
        }
        CommandOption *opt = NULL;
        LIST_FOREACH(options, opt, command->command_options) {
                if (!(opt->option_type->option_flag & method->supported_options)) {
                        fprintf(stderr,
                                "Unsupported option '%s' for method '%s'\n",
                                opt->option_type->option_long,
                                method->name);
                        return -EINVAL;
                }
        }

        return method->dispatch(command, userdata);
}

void command_option_free(CommandOption *option) {
        free_and_null(option->value);
        free_and_null(option);
}

char *command_get_option(Command *command, int key) {
        CommandOption *option = NULL;
        LIST_FOREACH(options, option, command->command_options) {
                if (key == option->key) {
                        return option->value;
                }
        }
        return NULL;
}

int command_get_option_long(Command *command, int key, long *ret) {
        const char *opt = command_get_option(command, key);
        long val = 0;
        if (opt != NULL && !streq(opt, "") && !parse_long(opt, &val)) {
                return -EINVAL;
        }
        *ret = val;
        return 0;
}

bool command_flag_exists(Command *command, int key) {
        CommandOption *option = NULL;
        LIST_FOREACH(options, option, command->command_options) {
                if (key == option->key) {
                        return true;
                }
        }
        return false;
}

int command_add_option(Command *command, int key, char *value, const OptionType *option_type) {
        _cleanup_command_option_ CommandOption *opt = malloc0(sizeof(CommandOption));
        if (opt == NULL) {
                return -ENOMEM;
        }

        opt->key = key;
        if (value != NULL) {
                opt->is_flag = false;
                opt->value = strdup(value);
                if (opt->value == NULL) {
                        return -ENOMEM;
                }
        } else {
                opt->is_flag = true;
                opt->value = NULL;
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wanalyzer-malloc-leak"
        opt->option_type = option_type;

        LIST_APPEND(options, command->command_options, steal_pointer(&opt));
#pragma GCC diagnostic pop

        return 0;
}

const OptionType *get_option_type(const OptionType option_types[], int option_short) {
        assert(option_types);

        for (size_t i = 0; option_types[i].option_flag; i++) {
                if (option_short == option_types[i].option_short) {
                        return option_types + i;
                }
        }
        return NULL;
}

const Method *methods_get_method(char *name, const Method methods[]) {
        assert(methods);
        if (name == NULL) {
                return NULL;
        }

        for (size_t i = 0; methods[i].dispatch; i++) {
                if (streq(name, methods[i].name)) {
                        return methods + i;
                }
        }

        return NULL;
}
