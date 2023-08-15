/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <systemd/sd-event.h>

typedef uint64_t usec_t;

static inline usec_t usec_add(usec_t a, usec_t b) {

        if (a > UINT64_MAX - b) {
                return UINT64_MAX;
        }

        return a + b;
}

static int event_reset_time(
                sd_event *event,
                sd_event_source **event_source,
                clockid_t clock,
                uint64_t usec,
                uint64_t accuracy,
                sd_event_time_handler_t callback,
                void *userdata,
                int64_t priority,
                const char *description,
                bool force_reset) {

        int enabled = 0;
        int r = 0;
        clockid_t c = 0;

        assert(event);
        assert(event_source);

        if (*event_source) {
                if (!force_reset) {
                        r = sd_event_source_get_enabled(*event_source, &enabled);
                        if (r < 0) {
                                return r;
                        }

                        if (enabled != SD_EVENT_OFF) {
                                return 0;
                        }
                }

                r = sd_event_source_get_time_clock(*event_source, &c);
                if (r < 0) {
                        return r;
                }

                if (c != clock) {
                        return r;
                }

                r = sd_event_source_set_time(*event_source, usec);
                if (r < 0) {
                        return r;
                }

                r = sd_event_source_set_time_accuracy(*event_source, accuracy);
                if (r < 0) {
                        return r;
                }

                if (sd_event_source_set_userdata(*event_source, userdata) == NULL) {
                        return -1;
                }

                r = sd_event_source_set_enabled(*event_source, SD_EVENT_ONESHOT);
                if (r < 0) {
                        return r;
                }
        } else {
                r = sd_event_add_time(event, event_source, clock, usec, accuracy, callback, userdata);
                if (r < 0) {
                        return r;
                }
        }

        r = sd_event_source_set_priority(*event_source, priority);
        if (r < 0) {
                return r;
        }

        if (description) {
                r = sd_event_source_set_description(*event_source, description);
                if (r < 0) {
                        return r;
                }
        }

        return 0;
}

int event_reset_time_relative(
                sd_event *e,
                sd_event_source **s,
                clockid_t clock,
                uint64_t usec,
                uint64_t accuracy,
                sd_event_time_handler_t callback,
                void *userdata,
                int64_t priority,
                const char *description,
                bool force_reset) {

        usec_t usec_now = 0;
        int r = 0;

        assert(e);

        r = sd_event_now(e, clock, &usec_now);
        if (r < 0) {
                return r;
        }

        return event_reset_time(
                        e,
                        s,
                        clock,
                        usec_add(usec_now, usec),
                        accuracy,
                        callback,
                        userdata,
                        priority,
                        description,
                        force_reset);
}
