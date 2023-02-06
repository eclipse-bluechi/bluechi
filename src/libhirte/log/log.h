#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <systemd/sd-journal.h>

#include "libhirte/common/common.h"

typedef enum LogLevel {
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
} LogLevel;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static const char *log_level_strings[] = { "DEBUG", "INFO", "WARN", "ERROR" };

const char *log_level_to_string(LogLevel l) {
        return log_level_strings[l];
}


typedef void (*LogFn)(LogLevel lvl, const char *msg, const char *data);

void hirte_log_to_journald(LogLevel lvl, const char *msg, const char *data) {
        // clang-format off
        sd_journal_send(
                "HIRTE_LOG_LEVEL=%s", log_level_to_string(lvl),
                "HIRTE_MESSAGE=%s", msg,
                "HIRTE_DATA=%s", data,
                NULL);
        // clang-format on
}

void hirte_log_to_stdout(LogLevel lvl, const char *msg, const char *data) {
        time_t t = time(NULL);
        const size_t timestamp_size = 9;
        char timebuf[timestamp_size];
        timebuf[strftime(timebuf, sizeof(timebuf), "%H:%M:%S", localtime(&t))] = '\0';

        // clang-format off
        printf("%s %s\t{msg: %s, data: %s}\n", 
                timebuf, log_level_to_string(lvl), 
                msg, data);
        // clang-format on
}


// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static struct {
        LogFn log_fn;
        LogLevel level;
        bool is_quiet;
} HirteLogConfig;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

void hirte_log_set_log_fn(LogFn log_fn) {
        HirteLogConfig.log_fn = log_fn;
}

void hirte_log_set_level(LogLevel level) {
        HirteLogConfig.level = level;
}

void hirte_log_set_quiet(bool is_quiet) {
        HirteLogConfig.is_quiet = is_quiet;
}

bool shouldLog(LogLevel lvl) {
        return (HirteLogConfig.level <= lvl && !HirteLogConfig.is_quiet && HirteLogConfig.log_fn != NULL);
}


#define hirte_log(lvl, msg, ...)                     \
        if (shouldLog(lvl)) {                        \
                HirteLogConfig.log_fn(lvl, msg, ""); \
        }

#define hirte_log_debug(msg) hirte_log(LOG_LEVEL_DEBUG, msg)
#define hirte_log_info(msg) hirte_log(LOG_LEVEL_INFO, msg)
#define hirte_log_warn(msg) hirte_log(LOG_LEVEL_WARN, msg)
#define hirte_log_error(msg) hirte_log(LOG_LEVEL_ERROR, msg)


#define hirte_logf(lvl, msgfmt, ...)                 \
        if (shouldLog(lvl)) {                        \
                _cleanup_free_ char *msg = NULL;     \
                asprintf(&msg, msgfmt, __VA_ARGS__); \
                HirteLogConfig.log_fn(lvl, msg, ""); \
        }

#define hirte_log_debugf(fmt, ...) hirte_logf(LOG_LEVEL_DEBUG, fmt, __VA_ARGS__)
#define hirte_log_infof(fmt, ...) hirte_logf(LOG_LEVEL_INFO, fmt, __VA_ARGS__)
#define hirte_log_warnf(fmt, ...) hirte_logf(LOG_LEVEL_WARN, fmt, __VA_ARGS__)
#define hirte_log_errorf(fmt, ...) hirte_logf(LOG_LEVEL_ERROR, fmt, __VA_ARGS__)


#define hirte_log_with_data(lvl, msg, datafmt, ...)    \
        if (shouldLog(lvl)) {                          \
                _cleanup_free_ char *data = NULL;      \
                asprintf(&data, datafmt, __VA_ARGS__); \
                HirteLogConfig.log_fn(lvl, msg, data); \
        }

#define hirte_log_debug_with_data(msg, datafmt, ...) \
        hirte_log_with_data(LOG_LEVEL_DEBUG, msg, datafmt, __VA_ARGS__)
#define hirte_log_info_with_data(msg, datafmt, ...) \
        hirte_log_with_data(LOG_LEVEL_INFO, msg, datafmt, __VA_ARGS__)
#define hirte_log_warn_with_data(msg, datafmt, ...) \
        hirte_log_with_data(LOG_LEVEL_WARN, msg, datafmt, __VA_ARGS__)
#define hirte_log_error_with_data(msg, datafmt, ...) \
        hirte_log_with_data(LOG_LEVEL_ERROR, msg, datafmt, __VA_ARGS__)
