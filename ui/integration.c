/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "dryos.h"
#include "bmp.h"
#include "config.h"
#include "menu.h"
#include "lens.h"
#include "lvinfo.h"
#include "zebra.h"
#include "cropmarks.h"
#include "nordic.h"
#include "face.h"
#include "zh_translation.h"
static CONFIG_INT("camdesk.ui.style",cd_style,1);
extern int should_draw_bottom_bar(void);
int camdesk_ui_enabled(void){return cd_style==1;}
static int cd_font(int size,int color,int bg)
{
    int f=size>=40?FONT_DYN(5,CD_TEXT,CD_BG):size==32?FONT_LARGE:
        size==28?FONT_MED_LARGE:size==23?FONT_MED:size==20?FONT_MONO_20:FONT_SMALL;
    return FONT(f,color,bg);
}
static void paint(int c,int x,int y,int w,int h)
{
    bmp_fill(c,x,y,w,h);
    /* The native fill excludes the final row/column; complete our owned canvas. */
    if(x+w==CD_UI_W){uint8_t *v=bmp_vram();for(int yy=y;yy<y+h;yy++)bmp_putpixel_fast(v,CD_UI_W-1,yy,c);}
    if(y+h==CD_UI_H){uint8_t *v=bmp_vram();for(int xx=x;xx<x+w;xx++)bmp_putpixel_fast(v,xx,CD_UI_H-1,c);}
}
static void letters(int size,int c,int bg,int x,int y,const char *s)
{
    const char *translated=strncmp(s,"Tab ",4)==0?zh_text(s):s;
    if(!cd_face_paint(size,c,bg,x,y,translated,paint))
        bmp_printf(cd_font(size,c,bg),x,y,"%s",translated);
}
static int width(int size,const char *s)
{
    const char *translated=strncmp(s,"Tab ",4)==0?zh_text(s):s;int result=cd_face_width(size,translated);
    return result>=0?result:bmp_string_width(cd_font(size,CD_TEXT,CD_BG),translated);
}
static const struct cd_ui_port port={paint,letters,width};
int camdesk_ui_try_monitor(int top,int bottom)
{
    struct cd_ui_context c={camdesk_ui_enabled(),lv,get_global_draw(),gui_menu_shown(),
        is_movie_mode(),EXT_MONITOR_CONNECTED,lv_dispsize,0,0};
    if(c.enabled && c.live && c.global_draw && !c.menu && !c.movie && !c.external && c.zoom==1){
        c.idle=liveview_display_idle();c.bottom_ok=should_draw_bottom_bar();
    }
    if(!cd_ui_can_monitor(&c))return 0;
    struct cd_ui_snapshot snapshot;camdesk_ui_snapshot(&snapshot);
    cd_ui_monitor(&port,&snapshot,top,bottom);return 1;
}
void camdesk_ui_draw_dashboard(void)
{
    struct cd_ui_snapshot snapshot;camdesk_ui_snapshot(&snapshot);
    cd_ui_dashboard(&port,&snapshot);
}
void camdesk_ui_draw_tab(const char *name,int selected,int x,int w)
{
    const char *label=
        streq(name,"Home")?"Tab Home":streq(name,"Expo")?"Tab Expo":streq(name,"Overlay")?"Tab Overlay":
        streq(name,"Movie")?"Tab Movie":streq(name,"Shoot")?"Tab Shoot":streq(name,"Focus")?"Tab Focus":
        streq(name,"Display")?"Tab Display":streq(name,"Prefs")?"Tab Prefs":streq(name,"Modules")?"Tab Modules":
        streq(name,"Debug")?"Tab Debug":streq(name,"Help")?"Tab Help":streq(name,"MyMenu")?"Tab Favorites":
        streq(name,"Scripts")?"Tab Scripts":streq(name,"Games")?"Tab Games":streq(name,"Audio")?"Tab Audio":name;
    cd_ui_tab(&port,label,selected,x,w);
}
static MENU_SELECT_FUNC(style_change)
{
    menu_numeric_toggle(&cd_style,delta,0,1);
    crop_set_dirty(5);lens_display_set_dirty();redraw();
}
static struct menu_entry ui_settings[]={
    {.name="Interface style",.priv=&cd_style,.min=0,.max=1,
     .choices=CHOICES("Classic","Nordic"),.select=style_change,
     .help="Nordic: clean current-parameter page; turn a dial to open the full menu."},
};
static struct menu_entry status_entry[]={
    {.name="Camera status",.help="Current readings only. Turn a dial for the full menu."},
};
static void ui_init(void *unused)
{
    cd_face_init();
    menu_add("Prefs",ui_settings,COUNT(ui_settings));
    menu_add("Home",status_entry,COUNT(status_entry));
}
INIT_FUNC("camdesk_ui",ui_init);
