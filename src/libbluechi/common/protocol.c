/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "common.h"

const char *job_state_to_string(JobState s) {
        switch (s) {
        case JOB_WAITING:
                return "waiting";
        case JOB_RUNNING:
                return "running";
        default:
                return "invalid";
        }
}
JobState job_state_from_string(const char *s) {
        if (streq(s, "waiting")) {
                return JOB_WAITING;
        }
        if (streq(s, "running")) {
                return JOB_RUNNING;
        }
        return JOB_INVALID;
}

static const char * const unit_active_state_table[_UNIT_ACTIVE_STATE_MAX] = {
        [UNIT_ACTIVE] = "active",           [UNIT_RELOADING] = "reloading",
        [UNIT_INACTIVE] = "inactive",       [UNIT_FAILED] = "failed",
        [UNIT_ACTIVATING] = "activating",   [UNIT_DEACTIVATING] = "deactivating",
        [UNIT_MAINTENANCE] = "maintenance",
};

const char *active_state_to_string(UnitActiveState s) {
        if (s >= 0 && s < _UNIT_ACTIVE_STATE_MAX) {
                return unit_active_state_table[s];
        }
        return "invalid";
}

UnitActiveState active_state_from_string(const char *s) {
        for (int i = 0; i < _UNIT_ACTIVE_STATE_MAX; i++) {
                if (streq(unit_active_state_table[i], s)) {
                        return i;
                }
        }
        return _UNIT_ACTIVE_STATE_INVALID;
}
