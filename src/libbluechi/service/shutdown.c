/*
 * Copyright Contributors to the Eclipse BlueChi project
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include <errno.h>

#include "libbluechi/common/common.h"
#include "libbluechi/log/log.h"

#include "shutdown.h"

static int event_loop_signal_handler(
                sd_event_source *event_source, UNUSED const struct signalfd_siginfo *si, void *userdata) {
        if (event_source == NULL) {
                return -EINVAL;
        }

        ShutdownHook *hook = (ShutdownHook *) userdata;
        if (hook != NULL && hook->shutdown != NULL) {
                hook->shutdown(hook->userdata);
        }

        sd_event *event = sd_event_source_get_event(event_source);
        return sd_event_exit(event, 0);
}

int event_loop_add_shutdown_signals(sd_event *event, ShutdownHook *hook) {
        sigset_t sigset;
        int r = 0;

        if (event == NULL) {
                return -EINVAL;
        }

        // Block this thread from handling SIGTERM and SIGINT so that these
        // signals can be handled by the event loop instead.
        r = sigemptyset(&sigset);
        if (r < 0) {
                bc_log_errorf("sigemptyset() failed: %s", strerror(-r));
                return -1;
        }
        r = sigaddset(&sigset, SIGTERM);
        if (r < 0) {
                bc_log_errorf("sigaddset() failed: %s", strerror(-r));
                return -1;
        }
        r = sigaddset(&sigset, SIGINT);
        if (r < 0) {
                bc_log_errorf("sigaddset() failed: %s", strerror(-r));
                return -1;
        }
        r = sigprocmask(SIG_BLOCK, &sigset, NULL);
        if (r < 0) {
                bc_log_errorf("sigprocmask() failed: %s", strerror(-r));
                return -1;
        }

        // Add SIGTERM and SIGINT as event sources in the event loop.
        r = sd_event_add_signal(event, NULL, SIGTERM, event_loop_signal_handler, hook);
        if (r < 0) {
                bc_log_errorf("sd_event_add_signal() failed: %s", strerror(-r));
                return -1;
        }
        r = sd_event_add_signal(event, NULL, SIGINT, event_loop_signal_handler, hook);
        if (r < 0) {
                bc_log_errorf("sd_event_add_signal() failed: %s", strerror(-r));
                return -1;
        }

        return 0;
}
