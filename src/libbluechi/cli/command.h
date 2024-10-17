/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "libbluechi/common/common.h"

#define ARG_ANY (INT8_MAX)

typedef struct Command Command;
typedef struct CommandOption CommandOption;
typedef struct OptionType OptionType;
typedef struct Method Method;

struct Command {
        int ref_count;

        const Method *method;
        char *op;
        int opargc;
        char **opargv;

        bool is_help;

        LIST_HEAD(CommandOption, command_options);
};

Command *new_command();
void command_unref(Command *command);
DEFINE_CLEANUP_FUNC(Command, command_unref)
#define _cleanup_command_ _cleanup_(command_unrefp)

int command_execute(Command *command, void *userdata);

struct CommandOption {
        int key;
        char *value;
        bool is_flag;
        const OptionType *option_type;
        LIST_FIELDS(CommandOption, options);
};
void command_option_free(CommandOption *option);
DEFINE_CLEANUP_FUNC(CommandOption, command_option_free)
#define _cleanup_command_option_ _cleanup_(command_option_freep)

char *command_get_option(Command *command, int key);
int command_get_option_long(Command *command, int key, long *ret);
bool command_flag_exists(Command *command, int key);
int command_add_option(Command *command, int key, char *value, const OptionType *option_type);

struct OptionType {
        int option_short;
        char *option_long;
        uint64_t option_flag;
};

const OptionType *get_option_type(const OptionType option_types[], int option_short);

struct Method {
        char *name;
        int min_args, max_args;
        uint64_t supported_options;
        int (* const dispatch)(Command *command, void *userdata);
        void (* const usage)();
};

const Method *methods_get_method(char *name, const Method methods[]);
