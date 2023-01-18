#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// NOLINTNEXTLINE
static bool done = false;

static void signal_handler(int signal) {
        printf("\n%s(): Received signal %d\n", __func__, signal);
        done = true;
}

int main() {
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
