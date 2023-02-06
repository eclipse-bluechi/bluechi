#include <stdio.h>
#include <stdlib.h>

#include "libhirte/log/log.h"


int main() {
        hirte_log_set_level(LOG_LEVEL_INFO);
        hirte_log_set_quiet(false);
        hirte_log_set_log_fn(hirte_log_to_journald_with_location);

        hirte_log_debug("This is the debug log message will not appear");
        hirte_log_info("This is the info log message you are waiting for");
        hirte_log_warn("This is the warn log message you are waiting for");
        hirte_log_error("This is the error log message you are waiting for");

        hirte_log_debugf("This is the formatted %s log message will not appear", "debug");
        hirte_log_infof("This is the formatted %s log message you are waiting for", "info");
        hirte_log_warnf("This is the formatted %s log message you are waiting for", "warn");
        hirte_log_errorf("This is the formatted %s log message you are waiting for", "error");

        hirte_log_debug_with_data(
                        "This is the enriched debug log message that will not appear",
                        "{\"%s\": \"%s\"}",
                        "jsonkey",
                        "jsonvalue");
        hirte_log_info_with_data(
                        "This is the enriched info log message you are waiting for",
                        "{\"%s\": \"%s\"}",
                        "jsonkey",
                        "jsonvalue");
        hirte_log_warn_with_data(
                        "This is the enriched warn log message you are waiting for",
                        "{\"%s\": \"%s\"}",
                        "jsonkey",
                        "jsonvalue");
        hirte_log_error_with_data(
                        "This is the enriched error log message you are waiting for",
                        "{\"%s\": \"%s\"}",
                        "jsonkey",
                        "jsonvalue");

        exit(EXIT_SUCCESS);
}
