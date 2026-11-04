// SPDX-License-Identifier: GPL-2.0-or-later
#include <module.h>
#include <dryos.h>
#include <bmp.h>
#include <menu.h>
#include <shoot.h>
#include <propvalues.h>
#include "interval_setup.h"

static unsigned int input_count;
static struct camdesk_plan plan = { 5, 3, 30 };
static int owns_session;
static int session_limit;
static int last_count;
static int completed;

static const char *setting_names[] = {
    "Take a pic every", "Start trigger", "Start after", "Stop after", "Intervalometer"
};

static int backend_get(enum camdesk_setting key)
{
    return menu_get_value_from_script(key == CD_ENABLED ? "Shoot" : "Intervalometer", setting_names[key]);
}

static int backend_set(enum camdesk_setting key, int value)
{
    return menu_set_value_from_script(key == CD_ENABLED ? "Shoot" : "Intervalometer", setting_names[key], value);
}

static const struct camdesk_backend backend = { backend_get, backend_set };

static MENU_SELECT_FUNC(arm_plan)
{
    if (is_movie_mode() || is_hdr_bracketing_enabled()) {
        NotifyBox(4000, "Use photo mode; disable HDR bracketing first.");
        return;
    }
    enum camdesk_result result = camdesk_arm(&plan, &backend, is_intervalometer_running());
    if (result == CD_OK) {
        owns_session = 1;
        session_limit = plan.frames;
        last_count = completed = 0;
        NotifyBox(5000, "Armed. Exit menu, hold half-shutter, then release.");
    } else {
        const char *errors[] = {
            "Ready", "Invalid plan", "Stop the existing intervalometer first",
            "Required ML menu is unavailable", "Setup failed; previous settings restored",
            "Setup recovery failed: disable intervalometer and restart"
        };
        NotifyBox(5000, "%s", errors[result]);
    }
}

static MENU_SELECT_FUNC(stop_plan)
{
    int disabled = backend_set(CD_ENABLED, 0) == 1;
    intervalometer_stop();
    if (disabled) {
        owns_session = 0;
        completed = 0;
    }
    NotifyBox(4000, disabled ? "Intervalometer disabled." : "Disable intervalometer in Shoot menu.");
}

static MENU_UPDATE_FUNC(status_update)
{
    const int enabled = backend_get(CD_ENABLED);
    MENU_SET_VALUE("%s", completed ? "Complete" : is_intervalometer_running() ? "Running" :
                   enabled == 1 ? "Armed" : enabled == 0 ? "Idle" : "Unavailable");
}

static MENU_UPDATE_FUNC(progress_update)
{
    MENU_SET_VALUE("%d / %d", last_count, session_limit);
}

static unsigned int interval_done(unsigned int context)
{
    if (owns_session) {
        last_count = get_interval_count();
        if (last_count >= session_limit) {
            if (backend_set(CD_ENABLED, 0) == 1) {
                owns_session = 0;
                completed = 1;
            }
            intervalometer_stop();
        }
    }
    return 0;
}

static MENU_UPDATE_FUNC(version_update)
{
    MENU_SET_VALUE("0.4.2-hui");
}

static MENU_UPDATE_FUNC(target_update)
{
    MENU_SET_VALUE("50D / 1.0.9");
}

static MENU_SELECT_FUNC(input_check)
{
    if (++input_count >= 10000) input_count = 0;
}

static MENU_UPDATE_FUNC(input_update)
{
    MENU_SET_VALUE("%u", input_count);
}

static struct menu_entry camdesk_menu[] = {
    {
        .name = "CamDesk",
        .select = menu_open_submenu,
        .help = "Automatic shooting using the native ML intervalometer.",
        .children = (struct menu_entry[]) {
            {
                .name = "Session status", .update = status_update,
                .help = "Armed: exit menu, hold half-shutter, then release to start.",
            },
            {
                .name = "Interval (seconds)", .priv = &plan.interval,
                .min = 1, .max = 3600, .unit = UNIT_DEC,
                .help = "Time between cycles. Applies on the next Arm plan.",
            },
            {
                .name = "Delay (seconds)", .priv = &plan.delay,
                .min = 0, .max = 3600, .unit = UNIT_DEC,
                .help = "Initial delay after the trigger (native ML minimum: 1 sec).",
            },
            {
                .name = "Number of cycles", .priv = &plan.frames,
                .min = 1, .max = 5000, .unit = UNIT_DEC,
                .help = "Finite sequence length. Native shooting settings still apply.",
            },
            {
                .name = "Arm plan", .select = arm_plan, .icon_type = IT_ACTION,
                .help = "Apply settings; start later with half-shutter. Does not shoot now.",
            },
            {
                .name = "Stop / Disarm", .select = stop_plan, .icon_type = IT_ACTION,
                .help = "Disable ML intervalometer; an exposure in progress may finish.",
            },
            {
                .name = "Session progress", .update = progress_update,
                .help = "Native cycle counter; saved files are not independently verified.",
            },
            {
                .name = "CamDesk version",
                .update = version_update,
                .help = "Menu prototype. Camera testing is pending.",
            },
            {
                .name = "Development target",
                .update = target_update,
                .help = "Intended platform, not detected hardware.",
            },
            {
                .name = "Input check",
                .select = input_check,
                .update = input_update,
                .icon_type = IT_ACTION,
                .help = "Select to increment the session counter (wraps at 10000).",
            },
            MENU_EOL,
        },
    },
};

static unsigned int camdesk_init(void)
{
    input_count = 0;
    owns_session = session_limit = last_count = completed = 0;
    menu_add("Prefs", camdesk_menu, COUNT(camdesk_menu));
    return 0;
}

static unsigned int camdesk_deinit(void)
{
    if (owns_session) {
        if (backend_set(CD_ENABLED, 0) != 1) return 1;
        intervalometer_stop();
        owns_session = 0;
    }
    menu_remove("Prefs", camdesk_menu, COUNT(camdesk_menu));
    return 0;
}

MODULE_INFO_START()
    MODULE_INIT(camdesk_init)
    MODULE_DEINIT(camdesk_deinit)
MODULE_INFO_END()

MODULE_CBRS_START()
    MODULE_CBR(CBR_INTERVALOMETER, interval_done, 0)
MODULE_CBRS_END()
