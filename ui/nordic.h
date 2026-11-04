/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CAMDESK_NORDIC_H
#define CAMDESK_NORDIC_H

/* This layer only paints; it never changes exposure or camera properties. */
enum { CD_UI_W=720, CD_UI_H=480, CD_UI_TOP_H=32, CD_UI_BOTTOM_Y=384 };
enum { CD_BG=2, CD_PANEL=40, CD_LINE=50, CD_TEXT=79, CD_MUTED=69,
       CD_ACCENT=19, CD_ALERT=8 };
struct cd_ui_snapshot {
    char mode[16], shutter[32], aperture[32], iso[40];
    char focus[12], quality[32], card[32], wb[48];
    char ec[16], ev[16];
    int ec_eighths, drive_mode, metering_mode;
    int battery_bars; /* actual native level, never an invented percentage */
    int peaking;
};
struct cd_ui_port {
    void (*fill)(int color,int x,int y,int w,int h);
    void (*text)(int size,int color,int bg,int x,int y,const char *text);
    int (*width)(int size,const char *text);
};
struct cd_ui_context {
    int enabled, live, global_draw, menu, movie, external, zoom, idle, bottom_ok;
};
int cd_ui_can_monitor(const struct cd_ui_context *c);
void cd_ui_monitor(const struct cd_ui_port *p,const struct cd_ui_snapshot *s,int top,int bottom);
void cd_ui_dashboard(const struct cd_ui_port *p,const struct cd_ui_snapshot *s);
void cd_ui_tab(const struct cd_ui_port *p,const char *label,int selected,int x,int width);
/* The status page consumes controls; a wheel tick only opens the menu page. */
struct cd_ui_pages { volatile int front; };
enum { CD_PAGE_OTHER,CD_PAGE_WHEEL,CD_PAGE_CLOSE };
enum { CD_PAGE_PASS,CD_PAGE_CONSUMED,CD_PAGE_TO_MENUS };
void cd_ui_pages_open(struct cd_ui_pages *pages,int enabled);
int cd_ui_pages_status(const struct cd_ui_pages *pages,int enabled,int home);
int cd_ui_pages_key(struct cd_ui_pages *pages,int enabled,int home,int key);

/* Platform adapter / read-only snapshot provider. */
int camdesk_ui_enabled(void);
int camdesk_ui_try_monitor(int top,int bottom);
void camdesk_ui_snapshot(struct cd_ui_snapshot *s);
void camdesk_ui_draw_tab(const char *name,int selected,int x,int width);
void camdesk_ui_draw_dashboard(void);
#endif
