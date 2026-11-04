#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic.h"
#define streq(a,b) (strcmp((a),(b))==0)
#define WB_KELVIN 9
struct lvinfo_item {char *value;};
struct {int wb_mode,kelvin,ae,raw_shutter,raw_aperture;} lens_info={0,0,0,112,40};
static int lv=1,camdesk_wb_mode_ph,wb_kelvin_ph,drive_mode,metering_mode;
#define APEX10_RAW2TV(raw) (((raw)-56)*10/8)
#define APEX10_RAW2AV(raw) (((raw)-8)*10/8)
static int battery_level_bars=2,card_present=1,peaking=1;
static const char *mode="M",*tv="\x82""125",*av="\x81""5.6",*iso="\x80""400",*wb="AutoWB";
#define CB(name,value_) static void name(struct lvinfo_item *item,int unused){(void)unused;item->value=(char*)(value_);}
CB(mode_update,mode)
CB(tv_update,tv)
CB(av_update,av)
CB(iso_update,iso)
CB(picq_update,"RAW")
CB(wb_update,wb)
CB(free_space_update,"15.0GB")
static int is_manual_focus(void){return 1;}
static void *get_shooting_card(void){return card_present?(void*)(uintptr_t)1:NULL;}
static int is_focus_peaking_enabled(void){return peaking;}
#include "../ui/lens_snapshot.inc"
int main(void)
{
    struct cd_ui_snapshot s;camdesk_ui_snapshot(&s);
    assert(streq(s.mode,"M") && streq(s.shutter,"1/125"));
    assert(streq(s.aperture,"f/5.6") && streq(s.iso,"400"));
    assert(streq(s.focus,"MF") && streq(s.wb,"AWB"));
    assert(streq(s.card,"CF 15.0GB") && s.battery_bars==2 && s.peaking);
    assert(streq(s.ec,"0.0") && streq(s.ev,"11.0"));
    lens_info.ae=-3;drive_mode=1;metering_mode=4;camdesk_wb_mode_ph=WB_KELVIN;wb_kelvin_ph=6500;
    lv=0;camdesk_ui_snapshot(&s);
    assert(streq(s.wb,"6500K") && streq(s.ec,"-0.4") && s.ec_eighths==-3);
    assert(s.drive_mode==1 && s.metering_mode==4);
    tv="BULB";camdesk_ui_snapshot(&s);assert(streq(s.ev,"--"));tv="\x82""125";
    camdesk_wb_mode_ph=1;camdesk_ui_snapshot(&s);assert(streq(s.wb,"DAY"));
    lv=1;camdesk_ui_snapshot(&s);assert(streq(s.wb,"AWB"));
    /* A fresh capture must reflect current native callback values. */
    tv="\x82""4000";av="";iso="\x80""12800";battery_level_bars=0;peaking=0;lens_info.raw_aperture=0;
    camdesk_ui_snapshot(&s);
    assert(streq(s.shutter,"1/4000") && streq(s.aperture,"M.AP") && streq(s.iso,"12800"));
    assert(!s.peaking && s.battery_bars==0);
    assert(streq(s.ev,"--"));
    lens_info.raw_aperture=40;camdesk_ui_snapshot(&s);assert(streq(s.ev,"--"));
    mode="[]";iso="\x80""Auto";lens_info.wb_mode=WB_KELVIN;lens_info.kelvin=5700;card_present=0;
    camdesk_ui_snapshot(&s);
    assert(streq(s.mode,"A") && streq(s.iso,"A") && streq(s.wb,"5700K") && streq(s.card,"CF --"));
    wb="UniWB";camdesk_ui_snapshot(&s);assert(streq(s.wb,"UNI"));
    puts("Snapshot: photo/live WB and Kelvin, signed EC, EV validity, drive/metering, fresh readings and M.AP passed.");
}
