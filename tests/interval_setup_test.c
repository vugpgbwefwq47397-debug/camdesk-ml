// SPDX-License-Identifier: GPL-2.0-or-later
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include "interval_setup.h"

static int settings[CD_SETTING_COUNT];
static int writes, fail_at, fail_rollback, missing;
static int read_setting(enum camdesk_setting key) { return missing == (int)key ? INT_MIN : settings[key]; }
static int write_setting(enum camdesk_setting key, int value)
{
    writes++;
    settings[key] = value; /* Simulate a partial write even on failure. */
    return writes == fail_at || (fail_rollback && writes > fail_at) ? 0 : 1;
}
static const struct camdesk_backend backend = { read_setting, write_setting };
static void reset(void)
{
    const int initial[] = { 10, 0, 7, 100, 0 };
    for (int i = 0; i < CD_SETTING_COUNT; i++) settings[i] = initial[i];
    writes = fail_at = fail_rollback = 0;
    missing = -1;
}
int main(void)
{
    struct camdesk_plan plan = { 5, 3, 30 };
    reset();
    assert(camdesk_arm(&plan, &backend, 0) == CD_OK);
    assert(settings[CD_INTERVAL] == 5 && settings[CD_TRIGGER] == 1);
    assert(settings[CD_DELAY] == 3 && settings[CD_LIMIT] == 30 && settings[CD_ENABLED] == 1);

    reset(); settings[CD_ENABLED] = 1;
    assert(camdesk_arm(&plan, &backend, 0) == CD_BUSY && writes == 0);
    reset(); assert(camdesk_arm(&plan, &backend, 1) == CD_BUSY && writes == 0);
    for (int key = 0; key < CD_SETTING_COUNT; key++) {
        reset(); missing = key;
        assert(camdesk_arm(&plan, &backend, 0) == CD_UNAVAILABLE && writes == 0);
    }
    for (int step = 1; step <= CD_SETTING_COUNT; step++) {
        reset(); fail_at = step;
        assert(camdesk_arm(&plan, &backend, 0) == CD_WRITE_FAILED);
        assert(settings[0] == 10 && settings[1] == 0 && settings[2] == 7);
        assert(settings[3] == 100 && settings[4] == 0);
    }
    reset(); fail_at = 2; fail_rollback = 1;
    assert(camdesk_arm(&plan, &backend, 0) == CD_RECOVERY_REQUIRED);
    const struct camdesk_plan invalid[] = {
        {0, 3, 30}, {3601, 3, 30}, {5, -1, 30}, {5, 3601, 30}, {5, 3, 0}, {5, 3, 5001}
    };
    for (unsigned int i = 0; i < sizeof(invalid)/sizeof(invalid[0]); i++) {
        reset(); assert(camdesk_arm(&invalid[i], &backend, 0) == CD_INVALID && writes == 0);
    }
    reset();
    const struct camdesk_plan boundary = { 3600, 0, 5000 };
    assert(camdesk_arm(&boundary, &backend, 0) == CD_OK);
    puts("interval_setup: success, busy, missing API, partial-write rollback and bounds passed");
    return 0;
}
