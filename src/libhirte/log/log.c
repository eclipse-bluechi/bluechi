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

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
struct HirteLogConfig config;

void hirte_log_set_log_fn(LogFn log_fn) {
        config.log_fn = log_fn;
}

void hirte_log_set_level(LogLevel level) {
        config.level = level;
}

void hirte_log_set_quiet(bool is_quiet) {
        config.is_quiet = is_quiet;
}

bool shouldLog(LogLevel lvl) {
        return (config.level <= lvl && !config.is_quiet && config.log_fn != NULL);
}