/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <errno.h>

#include "libbluechi/common/common.h"
#include "libbluechi/common/time-util.h"

#include "log.h"


static const char * const log_level_strings[] = { "DEBUG", "INFO", "WARN", "ERROR" };

const char *log_level_to_string(LogLevel l) {
        return log_level_strings[l];
}

LogLevel string_to_log_level(const char *l) {
        if (l == NULL) {
                return LOG_LEVEL_INVALID;
        }

        if (streq(log_level_strings[0], l)) {
                return LOG_LEVEL_DEBUG;
        }
        if (streq(log_level_strings[1], l)) {
                return LOG_LEVEL_INFO;
        }
        if (streq(log_level_strings[2], l)) {
                return LOG_LEVEL_WARN;
        }
        if (streq(log_level_strings[3], l)) {
                return LOG_LEVEL_ERROR;
        }

        return LOG_LEVEL_INVALID;
}

static int log_level_syslog[] = { LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERR };

static int log_level_to_syslog(LogLevel l) {
        if (l > LOG_LEVEL_ERROR) {
                l = LOG_LEVEL_ERROR;
        }
        return log_level_syslog[l];
}

int bc_log_to_journald_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data) {
        int r = 0;
        // file and line are required to contain the field names and func gets its key
        // automatically set (in contrast to what is written in the documentation).
        // see https://www.freedesktop.org/software/systemd/man/sd_journal_print.html
        _cleanup_free_ char *file_info = NULL;
        _cleanup_free_ char *line_info = NULL;
        r = asprintf(&file_info, "CODE_FILE=%s", file);
        if (r < 0) {
                return r;
        }
        r = asprintf(&line_info, "CODE_LINE=%s", line);
        if (r < 0) {
                return r;
        }

        int syslog_prio = log_level_to_syslog(lvl);

        // clang-format off
        if (data && *data != 0) {
                sd_journal_send_with_location(file_info, line_info, func,
                                              "MESSAGE=%s", msg,
                                              "PRIORITY=%i", syslog_prio,
                                              "DATA=%s", data,
                                              NULL);
        } else {
                sd_journal_send_with_location(file_info, line_info, func,
                                              "MESSAGE=%s", msg,
                                              "PRIORITY=%i", syslog_prio,
                                              NULL);
        }
        // clang-format on
        return 0;
}

int bc_log_to_stderr_full_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data) {
        _cleanup_free_ char *timestamp = get_formatted_log_timestamp();

        // clang-format off
        if (data && *data) {
                fprintf(stderr, "%s %s\t%s:%s %s\tmsg=\"%s\", data=\"%s\"\n",
                        timestamp, log_level_to_string(lvl),
                        file, line, func,
                        msg, data);
        } else {
                fprintf(stderr, "%s %s\t%s:%s %s\tmsg=\"%s\"\n",
                        timestamp, log_level_to_string(lvl),
                        file, line, func,
                        msg);
        }
        // clang-format on
        return 0;
}

int bc_log_to_stderr_with_location(
                LogLevel lvl,
                UNUSED const char *file,
                UNUSED const char *line,
                UNUSED const char *func,
                const char *msg,
                UNUSED const char *data) {
        _cleanup_free_ char *timestamp = get_formatted_log_timestamp();
        fprintf(stderr, "%s %s\t: %s\n", timestamp, log_level_to_string(lvl), msg);
        return 0;
}


static struct BcLogConfig {
        LogFn log_fn;
        LogLevel level;
        bool is_quiet;
} BcLogConfig;

void bc_log_set_log_fn(LogFn log_fn) {
        BcLogConfig.log_fn = log_fn;
}

LogFn bc_log_get_log_fn() {
        return BcLogConfig.log_fn;
}

void bc_log_set_level(LogLevel level) {
        BcLogConfig.level = level;
}

LogLevel bc_log_get_level() {
        return BcLogConfig.level;
}

void bc_log_set_quiet(bool is_quiet) {
        BcLogConfig.is_quiet = is_quiet;
}

bool bc_log_get_quiet() {
        return BcLogConfig.is_quiet;
}


int bc_log_init(struct config *config) {

        // set default logging settings
        bc_log_set_level(LOG_LEVEL_INFO);
        bc_log_set_log_fn(bc_log_to_journald_with_location);
        bc_log_set_quiet(false);

        LogLevel level = string_to_log_level(cfg_get_value(config, CFG_LOG_LEVEL));
        if (level != LOG_LEVEL_INVALID) {
                bc_log_set_level(level);
        }

        const char *target = cfg_get_value(config, CFG_LOG_TARGET);
        if (target != NULL) {
                if (streqi(BC_LOG_TARGET_JOURNALD, target)) {
                        bc_log_set_log_fn(bc_log_to_journald_with_location);
                } else if (streqi(BC_LOG_TARGET_STDERR, target)) {
                        bc_log_set_log_fn(bc_log_to_stderr_with_location);
                } else if (streqi(BC_LOG_TARGET_STDERR_FULL, target)) {
                        bc_log_set_log_fn(bc_log_to_stderr_full_with_location);
                }
        }

        const char *quiet = cfg_get_value(config, CFG_LOG_IS_QUIET);
        if (quiet != NULL) {
                bc_log_set_quiet(cfg_get_bool_value(config, CFG_LOG_IS_QUIET));
        }

        return 0;
}

bool shouldLog(LogLevel lvl) {
        return (BcLogConfig.level <= lvl && !BcLogConfig.is_quiet && BcLogConfig.log_fn != NULL);
}

int bc_log(LogLevel lvl, const char *file, const char *line, const char *func, const char *msg, const char *data) {
        if (shouldLog(lvl)) {
                return BcLogConfig.log_fn(lvl, file, line, func, msg, data);
        }
        return 0;
}

int bc_logf(LogLevel lvl, const char *file, const char *line, const char *func, const char *msgfmt, ...) {
        if (shouldLog(lvl)) {
                _cleanup_free_ char *msg = NULL;

                va_list argp;
                va_start(argp, msgfmt);
                int r = vasprintf(&msg, msgfmt, argp);
                if (r < 0) {
                        va_end(argp);
                        return r;
                }
                va_end(argp);

                return BcLogConfig.log_fn(lvl, file, line, func, msg, "");
        }
        return 0;
}

int bc_log_with_data(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *datafmt,
                ...) {
        if (shouldLog(lvl)) {
                _cleanup_free_ char *data = NULL;

                va_list argp;
                va_start(argp, datafmt);
                int r = vasprintf(&data, datafmt, argp);
                if (r < 0) {
                        va_end(argp);
                        return r;
                }
                va_end(argp);

                return BcLogConfig.log_fn(lvl, file, line, func, msg, data);
        }
        return 0;
}
