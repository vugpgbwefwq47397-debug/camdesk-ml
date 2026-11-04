// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef CAMDESK_INTERVAL_SETUP_H
#define CAMDESK_INTERVAL_SETUP_H

enum camdesk_setting { CD_INTERVAL, CD_TRIGGER, CD_DELAY, CD_LIMIT, CD_ENABLED, CD_SETTING_COUNT };
enum camdesk_result { CD_OK, CD_INVALID, CD_BUSY, CD_UNAVAILABLE, CD_WRITE_FAILED, CD_RECOVERY_REQUIRED };

struct camdesk_plan { int interval; int delay; int frames; };
struct camdesk_backend {
    int (*get)(enum camdesk_setting setting);
    int (*set)(enum camdesk_setting setting, int value);
};

/* Enable last, with a half-shutter trigger. Never take a picture here. */
enum camdesk_result camdesk_arm(const struct camdesk_plan *plan,
                               const struct camdesk_backend *backend, int running);
#endif
