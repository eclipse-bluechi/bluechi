#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../../libhirte/include/bus/user-bus.h"
#include "../../../libhirte/include/orchestrator.h"
#include "../../../libhirte/include/service/shutdown.h"

// NOLINTNEXTLINE
static bool done = false;

static void signal_handler(int signal) {
        printf("\n%s(): Received signal %d\n", __func__, signal);
        done = true;
}

int main() {
        sd_bus *user_bus = user_bus_open(NULL);
        service_call_shutdown(user_bus, ORCHESTRATOR_SERVICE_DEFAULT_NAME);

        struct sigaction sa;
        sa.sa_handler = &signal_handler;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGBUS, &sa, NULL);
        sigaction(SIGABRT, &sa, NULL);

        while (!done) {
                sleep(1);
                printf("%s(): Hello from client\n", __func__);
        }

        printf("%s(): Goodbye fromt client\n", __func__);

        exit(EXIT_SUCCESS);
}
