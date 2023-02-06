#include "log.h"

static const char * const log_level_strings[] = { "DEBUG", "INFO", "WARN", "ERROR" };

const char *log_level_to_string(LogLevel l) {
        return log_level_strings[l];
}


void hirte_log_to_journald_with_location(
                LogLevel lvl,
                const char *file,
                const char *line,
                const char *func,
                const char *msg,
                const char *data) {
        // file and line are required to contain the field names and func gets its key
        // automatically set (in contrast to what is written in the documentation).
        // see https://www.freedesktop.org/software/systemd/man/sd_journal_print.html
        _cleanup_free_ char *file_info = NULL;
        _cleanup_free_ char *line_info = NULL;
        asprintf(&file_info, "CODE_FILE=%s", file);
        asprintf(&line_info, "CODE_LINE=%s", line);

        // clang-format off
        sd_journal_send_with_location(
                file_info, line_info, func,
                "HIRTE_LOG_LEVEL=%s", log_level_to_string(lvl),
                "HIRTE_MESSAGE=%s", msg,
                "HIRTE_DATA=%s", data,
                NULL);
        // clang-format on
}

void hirte_log_to_stderr_with_location(
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
        fprintf(stderr, "%s %s\t%s:%s %s\t{msg: %s, data: %s}\n", 
                timebuf, log_level_to_string(lvl), 
                file, line, func,
                msg, data);
        // clang-format on
}

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static struct HirteLogConfig {
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

void hirte_log(LogLevel lvl,
               const char *file,
               const char *line,
               const char *func,
               const char *msg,
               const char *data) {
        if (shouldLog(lvl)) {
                HirteLogConfig.log_fn(lvl, file, line, func, msg, data);
        }
}

void hirte_logf(LogLevel lvl, const char *file, const char *line, const char *func, const char *msgfmt, ...) {
        if (shouldLog(lvl)) {
                _cleanup_free_ char *msg = NULL;

                va_list argp;
                va_start(argp, msgfmt);
                vasprintf(&msg, msgfmt, argp);
                va_end(argp);

                HirteLogConfig.log_fn(lvl, file, line, func, msg, "");
        }
}

void hirte_log_with_data(
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
                vasprintf(&data, datafmt, argp);
                va_end(argp);

                HirteLogConfig.log_fn(lvl, file, line, func, msg, data);
        }
}
