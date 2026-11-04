/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "nordic.h"

/* Fixed geometry is deliberately shared by the camera and host verification. */
static void box(const struct cd_ui_port *p,int c,int x,int y,int w,int h)
{
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>CD_UI_W)w=CD_UI_W-x;
    if(y+h>CD_UI_H)h=CD_UI_H-y;
    if(w>0 && h>0)p->fill(c,x,y,w,h);
}
static void text(const struct cd_ui_port *p,int size,int color,int bg,int x,int y,int width,const char *s)
{
    static const int sizes[]={160,128,96,64,56,40,32,28,23,20,12};
    for(unsigned i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++){
        if(sizes[i]>size)continue;
        if(y+sizes[i]>CD_UI_H)continue;
        if(p->width(sizes[i],s)<=width){p->text(sizes[i],color,bg,x,y,s);return;}
    }
    /* Refuse to paint a string that would overwrite an adjacent instrument. */
    if(p->width(12,"...")<=width)p->text(12,color,bg,x,y,"...");
}
int cd_ui_can_monitor(const struct cd_ui_context *c)
{
    return c && c->enabled && c->live && c->global_draw && !c->menu &&
        !c->movie && !c->external && c->zoom==1 && c->idle && c->bottom_ok;
}
static void exposure(const struct cd_ui_port *p,const struct cd_ui_snapshot *s,int y)
{
    const int edges[]={12,132,368,558,708};
    const char *values[]={s->mode,s->shutter,s->aperture,s->iso};
    for(int i=0;i<4;i++){
        int width=edges[i+1]-edges[i]-16;
        if(i==3)text(p,20,CD_MUTED,CD_BG,edges[i]+4,y+1,width,"ISO");
        text(p,i==3?56:64,CD_TEXT,CD_BG,edges[i]+4,y+25,width,
            values[i][0]?values[i]:(i==2?"M.AP":"--"));
        if(i)box(p,CD_LINE,edges[i]-6,y+12,1,72);
    }
}
void cd_ui_monitor(const struct cd_ui_port *p,const struct cd_ui_snapshot *s,int top,int bottom)
{
    if(top){
        box(p,CD_BG,0,0,720,CD_UI_TOP_H);
        text(p,20,CD_TEXT,CD_BG,14,4,60,s->focus);
        text(p,20,CD_TEXT,CD_BG,94,4,160,s->quality);
        int color=s->battery_bars<=0?CD_ALERT:CD_TEXT;
        box(p,color,640,8,58,18);box(p,CD_BG,642,10,54,14);box(p,color,698,13,4,8);
        /* Unknown levels have an empty outline; low/medium/full use real categories. */
        int count=s->battery_bars<0?0:s->battery_bars==0?1:s->battery_bars==1?2:3;
        for(int i=0;i<count;i++)box(p,color,646+16*i,12,12,10);
        box(p,CD_LINE,0,31,720,1);
    }
    if(bottom){
        box(p,CD_BG,0,CD_UI_BOTTOM_Y,720,96);
        box(p,CD_LINE,0,CD_UI_BOTTOM_Y,720,1);
        exposure(p,s,CD_UI_BOTTOM_Y);
    }
}
void cd_ui_tab(const struct cd_ui_port *p,const char *label,int selected,int x,int width)
{
    if(width<8)return;
    int size=width>=56?23:20;
    int tw=p->width(size,label);
    int tx=x+(width-tw)/2;
    if(tx<x+2)tx=x+2;
    text(p,size,selected?CD_ACCENT:CD_MUTED,CD_BG,tx,7,width-4,label);
    if(selected)box(p,CD_ACCENT,x+4,38,width-8,3);
}
static void line(const struct cd_ui_port *p,int c,int x,int y,int xx,int yy,int thick)
{
    int dx=xx-x,dy=yy-y,sx=dx<0?-1:1,sy=dy<0?-1:1;
    if(dx<0)dx=-dx;
    if(dy<0)dy=-dy;
    int error=dx-dy;
    for(;;){
        box(p,c,x,y,thick,thick);
        if(x==xx && y==yy)break;
        int e=2*error;
        if(e>-dy){error-=dy;x+=sx;}
        if(e<dx){error+=dx;y+=sy;}
    }
}
static void outline(const struct cd_ui_port *p,int c,int x,int y,int w,int h,int t)
{
    box(p,c,x,y,w,t);box(p,c,x,y+h-t,w,t);
    box(p,c,x,y,t,h);box(p,c,x+w-t,y,t,h);
}
/* Horizontal compensation makes round icons round on the 640x480 LCD. */
static void circle(const struct cd_ui_port *p,int c,int cx,int cy,int r,int filled)
{
    int rx=(r*9+4)/8;
    for(int y=-r;y<=r;y++)for(int x=-rx;x<=rx;x++){
        int d=x*x*r*r+y*y*rx*rx;
        if(d<=rx*rx*r*r && (filled || d>=(rx-2)*(rx-2)*(r-2)*(r-2)))
            box(p,c,cx+x,cy+y,1,1);
    }
}
static void wifi(const struct cd_ui_port *p,int c)
{
    /* Decorative only: EOS 50D has no built-in Wi-Fi. */
    line(p,c,28,18,35,13,2);line(p,c,35,13,44,12,2);
    line(p,c,44,12,53,13,2);line(p,c,53,13,60,18,2);
    line(p,c,33,22,39,18,2);line(p,c,39,18,49,18,2);line(p,c,49,18,55,22,2);
    line(p,c,38,26,44,23,2);line(p,c,44,23,50,26,2);circle(p,c,44,29,2,1);
}
static void subject(const struct cd_ui_port *p,int c)
{
    int x=242,y=59;
    line(p,c,x,y,x+11,y,2);line(p,c,x,y,x,y+10,2);
    line(p,c,x+42,y,x+53,y,2);line(p,c,x+53,y,x+53,y+10,2);
    line(p,c,x,y+27,x,y+37,2);line(p,c,x,y+37,x+11,y+37,2);
    line(p,c,x+53,y+27,x+53,y+37,2);line(p,c,x+42,y+37,x+53,y+37,2);
    circle(p,c,x+27,y+11,6,0);circle(p,c,x+13,y+17,4,0);circle(p,c,x+41,y+17,4,0);
    outline(p,c,x+17,y+22,22,10,2);
    line(p,c,x+7,y+24,x+14,y+24,2);line(p,c,x+7,y+24,x+7,y+30,2);
    line(p,c,x+40,y+24,x+47,y+24,2);line(p,c,x+47,y+24,x+47,y+30,2);
}
static void storage(const struct cd_ui_port *p,const struct cd_ui_snapshot *s)
{
    int x=575,y=421;
    outline(p,CD_TEXT,x,y,34,40,2);outline(p,CD_TEXT,x+5,y+7,34,40,2);
    outline(p,CD_TEXT,x+10,y+14,24,26,2);
    outline(p,CD_MUTED,x+43,y,38,47,2);
    line(p,CD_MUTED,x+47,y+39,x+56,y+26,2);line(p,CD_MUTED,x+56,y+26,x+75,y+40,2);
    circle(p,CD_MUTED,x+70,y+11,2,1);line(p,CD_MUTED,x+43,y,x+81,y+47,3);
    outline(p,CD_MUTED,666,421,43,21,2);
    text(p,12,CD_MUTED,CD_BG,671,425,34,"SSD");
    text(p,20,CD_MUTED,CD_BG,667,446,42,"CFe");
    /* RAW/JPEG is live; the second-media and SSD symbols are reference placeholders. */
    text(p,12,CD_TEXT,CD_BG,578,404,80,s->quality);
}
void cd_ui_dashboard(const struct cd_ui_port *p,const struct cd_ui_snapshot *s)
{
    box(p,CD_BG,0,0,720,480);
    wifi(p,CD_MUTED);
    circle(p,CD_TEXT,350,22,4,1);box(p,CD_LINE,363,19,19,6);
    int color=s->battery_bars==0?CD_ALERT:CD_TEXT;
    outline(p,color,653,15,44,15,2);box(p,color,697,19,4,7);
    int bars=s->battery_bars<0?0:s->battery_bars==0?1:s->battery_bars==1?2:3;
    for(int i=0;i<bars;i++)box(p,color,658+12*i,19,9,7);
    /* Restore the reference's full cell structure, including unsupported symbols. */
    box(p,CD_LINE,5,36,710,1);box(p,CD_LINE,5,113,710,1);
    const int top_edges[]={91,218,303};
    for(int i=0;i<3;i++)box(p,CD_LINE,top_edges[i],37,1,76);
    if(s->wb[0]=='A' && s->wb[1]=='W' && s->wb[2]=='B'){
        text(p,32,CD_TEXT,CD_BG,34,49,48,"A");text(p,20,CD_TEXT,CD_BG,30,78,54,"WB");
    }else{
        text(p,28,CD_TEXT,CD_BG,12,53,70,s->wb[0]?s->wb:"--");
        text(p,12,CD_MUTED,CD_BG,32,89,48,"WB");
    }
    box(p,CD_TEXT,124,59,61,38);
    text(p,32,CD_BG,CD_TEXT,130,61,53,s->focus[0]?s->focus:"AF");
    subject(p,CD_MUTED);
    const char *iso=s->iso[0]?s->iso:"--";
    /* Keep ISO right-aligned and its label beside the baseline at every ISO size. */
    int iso_size=64,iw=p->width(iso_size,iso);
    while(iw>280 && iso_size>20){iso_size=iso_size==64?56:iso_size==56?40:iso_size==40?28:20;iw=p->width(iso_size,iso);}
    if(iw>280)iw=280;
    int ix=704-iw;
    text(p,iso_size,CD_TEXT,CD_BG,ix,43+64-iso_size,iw,iso);
    text(p,23,CD_TEXT,CD_BG,ix-57,80,52,"ISO");
    const char *ap=s->aperture;
    if(!ap[0] || (ap[0]=='M' && ap[1]=='.'))text(p,96,CD_TEXT,CD_BG,20,143,332,"M.AP");
    else{
        if(ap[0]=='f')ap++;
        if(ap[0]=='/')ap++;
        text(p,56,CD_TEXT,CD_BG,20,137,68,"f/");
        text(p,160,CD_TEXT,CD_BG,85,123,267,ap);
    }
    text(p,128,CD_TEXT,CD_BG,402,133,302,s->shutter[0]?s->shutter:"--");
    /* AE-L remains an inactive reference icon; no unverified lock state is invented. */
    box(p,CD_MUTED,23,296,78,32);
    text(p,23,CD_BG,CD_MUTED,29,299,68,"AE-L");
    int pointer=s->ec_eighths;
    if(pointer<-24)pointer=-24;
    if(pointer>24)pointer=24;
    int px=112+(pointer*15)/4;
    const char *ec=s->ec[0]?s->ec:"--";
    int ew=p->width(23,ec),ex=px-ew/2;
    if(ex<15)ex=15;
    if(ex+ew>218)ex=218-ew;
    text(p,23,CD_TEXT,CD_BG,ex,340,203,ec);
    for(int i=-6;i<=6;i++)box(p,CD_TEXT,112+i*15,380-(i%2==0?12:8),2,i%2==0?12:8);
    box(p,CD_TEXT,px-1,362,3,19);
    text(p,23,CD_TEXT,CD_BG,242,359,46,"EV");
    text(p,40,CD_TEXT,CD_BG,290,345,210,s->ev[0]?s->ev:"--");
    text(p,12,CD_MUTED,CD_BG,565,383,142,s->card);
    box(p,CD_LINE,5,402,710,1);
    const int bottom_edges[]={88,173,259,552};
    for(int i=0;i<4;i++)box(p,CD_LINE,bottom_edges[i],403,1,69);
    text(p,56,CD_TEXT,CD_BG,21,412,64,s->mode[0]?s->mode:"--");
    /* Native drive: single circle / stacked continuous / timer hands. */
    circle(p,CD_TEXT,131,441,18,0);
    if(s->drive_mode==0)circle(p,CD_TEXT,131,441,12,1);
    else if(s->drive_mode==0x10 || s->drive_mode==0x11){
        line(p,CD_TEXT,131,441,131,429,2);line(p,CD_TEXT,131,441,140,447,2);
    }else outline(p,CD_TEXT,122,433,19,16,2);
    outline(p,CD_TEXT,194,426,43,29,2);
    if(s->metering_mode==1)circle(p,CD_TEXT,216,441,3,1);
    else if(s->metering_mode==4)circle(p,CD_TEXT,216,441,8,0);
    else if(s->metering_mode==3){circle(p,CD_TEXT,216,441,8,0);circle(p,CD_TEXT,216,441,2,1);}
    storage(p,s);
}

void cd_ui_pages_open(struct cd_ui_pages *pages,int enabled){pages->front=!!enabled;}
int cd_ui_pages_status(const struct cd_ui_pages *pages,int enabled,int home)
{return enabled && (pages->front || home);}
int cd_ui_pages_key(struct cd_ui_pages *pages,int enabled,int home,int key)
{
    if(!cd_ui_pages_status(pages,enabled,home))return CD_PAGE_PASS;
    if(key==CD_PAGE_CLOSE)return CD_PAGE_PASS;
    if(key==CD_PAGE_WHEEL){pages->front=0;return CD_PAGE_TO_MENUS;}
    return CD_PAGE_CONSUMED;
}
