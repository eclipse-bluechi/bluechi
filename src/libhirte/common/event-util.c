#include <stdint.h>
#include <systemd/sd-event.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t usec_t;

static inline usec_t usec_add(usec_t a, usec_t b) {

        if (a > UINT64_MAX - b)
                return UINT64_MAX;
        return a + b;
}

static int event_reset_time(
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

        bool created = false;
        int enabled, r;
        clockid_t c;

        assert(e);
        assert(s);

        if (*s) {
                if (!force_reset) {
                        r = sd_event_source_get_enabled(*s, &enabled);
                        if (r < 0)
                                return r;

                        if (enabled != SD_EVENT_OFF)
                                return 0;
                }

                r = sd_event_source_get_time_clock(*s, &c);
                if (r < 0)
                        return r;

                if (c != clock)
                        return r;

                r = sd_event_source_set_time(*s, usec);
                if (r < 0)
                        return r;

                r = sd_event_source_set_time_accuracy(*s, accuracy);
                if (r < 0)
                        return r;

                (void) sd_event_source_set_userdata(*s, userdata);

                r = sd_event_source_set_enabled(*s, SD_EVENT_ONESHOT);
                if (r < 0)
                        return r;
        } else {
                r = sd_event_add_time(e, s, clock, usec, accuracy, callback, userdata);
                if (r < 0)
                        return r;

                created = true;
        }

        r = sd_event_source_set_priority(*s, priority);
        if (r < 0)
                return r;

        if (description) {
                r = sd_event_source_set_description(*s, description);
                if (r < 0)
                        return r;
        }

        return created;
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

        usec_t usec_now;
        int r;

        assert(e);

        r = sd_event_now(e, clock, &usec_now);
        if (r < 0)
                return r;

        return event_reset_time(e, s, clock, usec_add(usec_now, usec), accuracy, callback, userdata, priority, description, force_reset);
}
