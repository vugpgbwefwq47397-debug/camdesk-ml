// SPDX-License-Identifier: GPL-2.0-or-later
#include <limits.h>
#include "interval_setup.h"

enum camdesk_result camdesk_arm(const struct camdesk_plan *plan,
                               const struct camdesk_backend *backend, int running)
{
    if (!plan || !backend || !backend->get || !backend->set ||
        plan->interval < 1 || plan->interval > 3600 ||
        plan->delay < 0 || plan->delay > 3600 ||
        plan->frames < 1 || plan->frames > 5000)
        return CD_INVALID;
    if (running) return CD_BUSY;

    int previous[CD_SETTING_COUNT];
    for (int i = 0; i < CD_SETTING_COUNT; i++) {
        previous[i] = backend->get((enum camdesk_setting)i);
        if (previous[i] == INT_MIN) return CD_UNAVAILABLE;
    }
    if (previous[CD_ENABLED]) return CD_BUSY;

    const int values[] = { plan->interval, 1, plan->delay, plan->frames, 1 };
    for (int i = 0; i < CD_SETTING_COUNT; i++) {
        if (backend->set((enum camdesk_setting)i, values[i]) != 1) {
            /* A failed adapter may have changed its value. Disable first. */
            int restored = backend->set(CD_ENABLED, 0) == 1;
            for (int j = 0; j < CD_ENABLED; j++)
                if (backend->set((enum camdesk_setting)j, previous[j]) != 1)
                    restored = 0;
            return restored ? CD_WRITE_FAILED : CD_RECOVERY_REQUIRED;
        }
    }
    return CD_OK;
}
