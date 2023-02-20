/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <errno.h>

#include "libhirte/common/common.h"
#include "libhirte/common/config.h"

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


int hirte_log_to_journald_with_location(
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

        // clang-format off
        sd_journal_send_with_location(
                file_info, line_info, func,
                "HIRTE_LOG_LEVEL=%s", log_level_to_string(lvl),
                "HIRTE_MESSAGE=%s", msg,
                "HIRTE_DATA=%s", data,
                NULL);
        // clang-format on
        return 0;
}

int hirte_log_to_stderr_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data) {

        time_t t = time(NULL);
        const size_t timestamp_size = 9;
        char timebuf[timestamp_size];
        timebuf[strftime(timebuf, sizeof(timebuf), "%H:%M:%S", localtime(&t))] = '\0';

        // clang-format off
        if (data && *data) {
                fprintf(stderr, "%s %s\t%s:%s %s\tmsg=\"%s\", data=\"%s\"\n",
                        timebuf, log_level_to_string(lvl),
                        file, line, func,
                        msg, data);
        } else {
                fprintf(stderr, "%s %s\t%s:%s %s\tmsg=\"%s\"\n",
                        timebuf, log_level_to_string(lvl),
                        file, line, func,
                        msg);
        }
        // clang-format on
        return 0;
}


static struct HirteLogConfig {
        LogFn log_fn;
        LogLevel level;
        bool is_quiet;
} HirteLogConfig;

void hirte_log_set_log_fn(LogFn log_fn) {
        HirteLogConfig.log_fn = log_fn;
}

void hirte_log_set_level(LogLevel level) {
        HirteLogConfig.level = level;
}

void hirte_log_set_quiet(bool is_quiet) {
        HirteLogConfig.is_quiet = is_quiet;
}

void hirte_log_init() {
        hirte_log_set_level(LOG_LEVEL_INFO);
        hirte_log_set_quiet(false);
        hirte_log_set_log_fn(hirte_log_to_stderr_with_location);
}

void hirte_log_init_from_env() {
        char *env_level = NULL;
        char *env_target = NULL;
        char *env_is_quiet = NULL;

        env_level = getenv(HIRTE_LOG_ENV_KEY_LEVEL);
        LogLevel level = string_to_log_level(env_level);
        if (level != LOG_LEVEL_INVALID) {
                hirte_log_set_level(level);
        }

        env_target = getenv(HIRTE_LOG_ENV_KEY_TARGET);
        if (env_target != NULL) {
                if (streqi(HIRTE_LOG_TARGET_JOURNALD, env_target)) {
                        hirte_log_set_log_fn(hirte_log_to_journald_with_location);
                } else if (streqi(HIRTE_LOG_TARGET_STDERR, env_target)) {
                        hirte_log_set_log_fn(hirte_log_to_stderr_with_location);
                }
        }

        env_is_quiet = getenv(HIRTE_LOG_ENV_KEY_IS_QUIET);
        if (is_true_value(env_is_quiet)) {
                hirte_log_set_quiet(true);
        } else if (is_false_value(env_is_quiet)) {
                hirte_log_set_quiet(false);
        }
}

int hirte_log_init_from_config(config *conf) {
        if (conf == NULL) {
                return -EINVAL;
        }

        topic *logging_topic = NULL;
        const char *target = NULL;
        const char *quiet = NULL;

        logging_topic = config_lookup_topic(conf, HIRTE_LOG_CONFIG_TOPIC);
        if (logging_topic == NULL) {
                return -EINVAL;
        }

        LogLevel level = string_to_log_level(topic_lookup(logging_topic, CFG_LOG_LEVEL));
        if (level != LOG_LEVEL_INVALID) {
                hirte_log_set_level(level);
        }

        target = topic_lookup(logging_topic, CFG_LOG_TARGET);
        if (target != NULL) {
                if (streqi(HIRTE_LOG_TARGET_JOURNALD, target)) {
                        hirte_log_set_log_fn(hirte_log_to_journald_with_location);
                } else if (streqi(HIRTE_LOG_TARGET_STDERR, target)) {
                        hirte_log_set_log_fn(hirte_log_to_stderr_with_location);
                }
        }

        quiet = topic_lookup(logging_topic, CFG_LOG_IS_QUIET);
        if (is_true_value(quiet)) {
                hirte_log_set_quiet(true);
        } else if (is_false_value(quiet)) {
                hirte_log_set_quiet(false);
        }

        return 0;
}

bool shouldLog(LogLevel lvl) {
        return (HirteLogConfig.level <= lvl && !HirteLogConfig.is_quiet && HirteLogConfig.log_fn != NULL);
}

int hirte_log(LogLevel lvl,
              const char *file,
              const char *line,
              const char *func,
              const char *msg,
              const char *data) {
        if (shouldLog(lvl)) {
                return HirteLogConfig.log_fn(lvl, file, line, func, msg, data);
        }
        return 0;
}

int hirte_logf(LogLevel lvl, const char *file, const char *line, const char *func, const char *msgfmt, ...) {
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

                return HirteLogConfig.log_fn(lvl, file, line, func, msg, "");
        }
        return 0;
}

int hirte_log_with_data(
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

                return HirteLogConfig.log_fn(lvl, file, line, func, msg, data);
        }
        return 0;
}
