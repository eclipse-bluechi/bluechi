/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>

#include <systemd/sd-journal.h>

#include "libbluechi/common/cfg.h"

#define BC_LOG_TARGET_JOURNALD "journald"
#define BC_LOG_TARGET_STDERR "stderr"
#define BC_LOG_TARGET_STDERR_FULL "stderr-full"

typedef enum LogLevel {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_INVALID,
} LogLevel;

const char *log_level_to_string(LogLevel l);
LogLevel string_to_log_level(const char *l);


typedef int (*LogFn)(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data);

int bc_log_to_journald_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data);
int bc_log_to_stderr_full_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data);
int bc_log_to_stderr_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data);

void bc_log_set_log_fn(LogFn log_fn);
void bc_log_set_level(LogLevel level);
void bc_log_set_quiet(bool is_quiet);
LogFn bc_log_get_log_fn();
LogLevel bc_log_get_level();
bool bc_log_get_quiet();

int bc_log_init(struct config *config);

bool shouldLog(LogLevel lvl);

int bc_log(LogLevel lvl, const char *file, const char *line, const char *func, const char *msg, const char *data);
int bc_logf(LogLevel lvl, const char *file, const char *line, const char *func, const char *msgfmt, ...);
int bc_log_with_data(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *datafmt,
                ...);

#define bc_log_debug(msg) bc_log(LOG_LEVEL_DEBUG, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, "")
#define bc_log_info(msg) bc_log(LOG_LEVEL_INFO, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, "")
#define bc_log_warn(msg) bc_log(LOG_LEVEL_WARN, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, "")
#define bc_log_error(msg) bc_log(LOG_LEVEL_ERROR, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, "")

#define bc_log_debugf(fmt, ...) \
        bc_logf(LOG_LEVEL_DEBUG, __FILE__, _SD_STRINGIFY(__LINE__), __func__, fmt, __VA_ARGS__)
#define bc_log_infof(fmt, ...) \
        bc_logf(LOG_LEVEL_INFO, __FILE__, _SD_STRINGIFY(__LINE__), __func__, fmt, __VA_ARGS__)
#define bc_log_warnf(fmt, ...) \
        bc_logf(LOG_LEVEL_WARN, __FILE__, _SD_STRINGIFY(__LINE__), __func__, fmt, __VA_ARGS__)
#define bc_log_errorf(fmt, ...) \
        bc_logf(LOG_LEVEL_ERROR, __FILE__, _SD_STRINGIFY(__LINE__), __func__, fmt, __VA_ARGS__)

#define bc_log_debug_with_data(msg, datafmt, ...) \
        bc_log_with_data(LOG_LEVEL_DEBUG, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, datafmt, __VA_ARGS__)
#define bc_log_info_with_data(msg, datafmt, ...) \
        bc_log_with_data(LOG_LEVEL_INFO, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, datafmt, __VA_ARGS__)
#define bc_log_warn_with_data(msg, datafmt, ...) \
        bc_log_with_data(LOG_LEVEL_WARN, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, datafmt, __VA_ARGS__)
#define bc_log_error_with_data(msg, datafmt, ...) \
        bc_log_with_data(LOG_LEVEL_ERROR, __FILE__, _SD_STRINGIFY(__LINE__), __func__, msg, datafmt, __VA_ARGS__)
