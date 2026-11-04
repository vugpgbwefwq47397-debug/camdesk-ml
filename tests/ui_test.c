#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nordic.h"
#include "cjk_font.h"
#include "zh_translation.h"
#include "face.h"

static unsigned char pixels[CD_UI_H][CD_UI_W];
static FILE *trace;
static int monitor_only;
static int glyph_painting;
static int instrument;
struct metric {int size;unsigned char widths[256];};
static struct metric metrics[]={{12,{0}},{20,{0}},{23,{0}},{28,{0}},{32,{0}},{40,{0}}};
static const char *display_text(const char *s){return strncmp(s,"Tab ",4)==0?zh_text(s):s;}
static void load_metrics(void)
{
    const char *names[]={"term12","term20","argnor23","argnor28","argnor32","cdnord40"};
    for(int i=0;i<6;i++){
        char path[256];snprintf(path,sizeof(path),i==5?"localization/fonts/%s.rbf":"upstream/ml/data/fonts/%s.rbf",names[i]);
        FILE *f=fopen(path,"rb");assert(f);uint32_t h[29];assert(fread(h,4,29,f)==29);
        assert(h[20]==(unsigned)metrics[i].size && h[22]<=h[23] && h[23]<256);
        assert(!fseek(f,h[25],SEEK_SET));assert(fread(metrics[i].widths+h[22],1,h[23]-h[22]+1,f)==h[23]-h[22]+1);fclose(f);
    }
}
static int measure(int size,const char *value)
{
    const char *s=display_text(value);int width=cd_face_width(size,s),index=0;
    if(width>=0)return width;
    width=0;
    while(index<6 && metrics[index].size!=size)index++;
    assert(index<6);
    while(*s){
        int step;uint32_t cp=utf8_decode(s,&step);s+=step;
        if(cp<128)width+=metrics[index].widths[cp];
        else {const char *bitmap;int cw,pw,gh;int a=cjk_glyph(cp,size,&bitmap,&cw,&pw,&gh);assert(a>0);width+=a;}
    }
    return width;
}
static void rect(int color,int x,int y,int w,int h)
{
    assert(x>=0 && y>=0 && w>0 && h>0 && x+w<=CD_UI_W && y+h<=CD_UI_H);
    if(monitor_only)assert(y+h<=CD_UI_TOP_H || y>=CD_UI_BOTTOM_Y);
    for(int yy=y;yy<y+h;yy++)memset(pixels[yy]+x,color,w);
    if(trace && !glyph_painting)fprintf(trace,"{\"kind\":\"fill\",\"color\":%d,\"x\":%d,\"y\":%d,\"w\":%d,\"h\":%d}\n",color,x,y,w,h);
}
static void string(int size,int color,int bg,int x,int y,const char *s)
{
    assert(x>=0 && y>=0 && x+measure(size,s)<=CD_UI_W && y+size<=CD_UI_H);
    if(monitor_only)assert(y+size<=CD_UI_TOP_H || y>=CD_UI_BOTTOM_Y);
    glyph_painting=1;
    int painted=cd_face_paint(size,color,bg,x,y,display_text(s),rect);
    if(instrument)assert(painted); /* instrument pages never use the old Latin bitmap face */
    if(!painted){
        int xx=x;
        for(const char *v=display_text(s);*v;){
            int step;uint32_t cp=utf8_decode(v,&step);v+=step;
            if(cp>=128){
                const char *bits;int cw,pw,gh;int advance=cjk_glyph(cp,size,&bits,&cw,&pw,&gh);assert(advance);
                rect(bg,xx,y,advance,gh);
                for(int yy=0;yy<gh;yy++)for(int a=0;a<pw;a++)
                    if((unsigned char)bits[yy*(cw/8)+a/8]&(1<<(a%8)))rect(color,xx+a,y+yy,1,1);
                xx+=advance;
            }else{
                char ch[2]={(char)cp,0};xx+=measure(size,ch);
            }
        }
    }
    glyph_painting=0;
    if(!trace)return;
    fprintf(trace,"{\"kind\":\"text\",\"size\":%d,\"color\":%d,\"bg\":%d,\"x\":%d,\"y\":%d,\"value\":\"",size,color,bg,x,y);
    for(const unsigned char *v=(const unsigned char*)display_text(s);*v;v++){
        if(*v=='"' || *v=='\\')fputc('\\',trace);
        assert(*v>=32);fputc(*v,trace);
    }
    fputs("\"}\n",trace);
}
static const struct cd_ui_port port={rect,string,measure};
static void gates(void)
{
    struct cd_ui_context c={1,1,1,0,0,0,1,1,1};assert(cd_ui_can_monitor(&c));
    assert(!cd_ui_can_monitor(NULL));
    for(int field=0;field<9;field++){
        struct cd_ui_context bad=c;
        switch(field){case 0:bad.enabled=0;break;case 1:bad.live=0;break;case 2:bad.global_draw=0;break;
        case 3:bad.menu=1;break;case 4:bad.movie=1;break;case 5:bad.external=1;break;
        case 6:bad.zoom=5;break;case 7:bad.idle=0;break;case 8:bad.bottom_ok=0;break;}
        assert(!cd_ui_can_monitor(&bad));
    }
}
static void frame(const char *name)
{
    char path[128];snprintf(path,sizeof(path),"build/ui-%s.idx",name);
    FILE *f=fopen(path,"wb");assert(f);assert(fwrite(pixels,1,sizeof(pixels),f)==sizeof(pixels));fclose(f);
}
static void face_validation(void)
{
    FILE *f=fopen("localization/fonts/cdneutral.cdf","rb");assert(f);
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
    unsigned char *data=malloc(n+1);assert(data);assert(fread(data,1,n,f)==(size_t)n);fclose(f);
    assert(cd_face_validate(data,n));
    assert(!cd_face_validate(data,n-1));assert(!cd_face_validate(data,n+1));
    const int offsets[]={0,4,8,12,16,18,20,22,24};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++){
        unsigned char old=data[offsets[i]];data[offsets[i]]^=1;
        assert(!cd_face_validate(data,n));data[offsets[i]]=old;
    }
    unsigned seed=1024;
    for(int i=0;i<2000;i++){
        seed=seed*1664525u+1013904223u;unsigned pos=seed%(unsigned)n;
        unsigned char old=data[pos];data[pos]^=(seed>>24)|1;
        (void)cd_face_validate(data,n);data[pos]=old;
    }
    free(data);
}
static void pages(void)
{
    struct cd_ui_pages p={0};
    assert(!cd_ui_pages_status(&p,1,0));
    cd_ui_pages_open(&p,1);assert(cd_ui_pages_status(&p,1,0));
    assert(cd_ui_pages_key(&p,1,0,CD_PAGE_OTHER)==CD_PAGE_CONSUMED);
    assert(cd_ui_pages_key(&p,1,0,CD_PAGE_CLOSE)==CD_PAGE_PASS);
    assert(cd_ui_pages_status(&p,1,0));
    assert(cd_ui_pages_key(&p,1,0,CD_PAGE_WHEEL)==CD_PAGE_TO_MENUS);
    assert(!cd_ui_pages_status(&p,1,0));
    assert(cd_ui_pages_key(&p,1,0,CD_PAGE_OTHER)==CD_PAGE_PASS);
    cd_ui_pages_open(&p,1);assert(cd_ui_pages_status(&p,1,0));
    cd_ui_pages_open(&p,0);assert(!cd_ui_pages_status(&p,0,1));
    assert(cd_ui_pages_key(&p,0,1,CD_PAGE_WHEEL)==CD_PAGE_PASS);
    assert(cd_ui_pages_status(&p,1,1));
    assert(cd_ui_pages_key(&p,1,1,CD_PAGE_WHEEL)==CD_PAGE_TO_MENUS);
}
int main(void)
{
    assert(cjk_fonts_init()>0);assert(cd_face_init());zh_enable(1);load_metrics();gates();pages();face_validation();
    struct cd_ui_snapshot s={.mode="M",.shutter="1/125",.aperture="f/5.6",.iso="400",.focus="MF",.quality="RAW",.card="CF 37.2GB",.wb="AWB",.ec="0.0",.ev="11.9",.metering_mode=3,.battery_bars=2,.peaking=1};
    monitor_only=1;instrument=1;memset(pixels,0xa5,sizeof(pixels));
    trace=fopen("build/ui-monitor.jsonl","w");assert(trace);cd_ui_monitor(&port,&s,1,1);fclose(trace);trace=0;
    frame("monitor");
    for(int y=32;y<384;y++)for(int x=0;x<720;x++)assert(pixels[y][x]==0xa5);
    memset(pixels,0xa5,sizeof(pixels));cd_ui_monitor(&port,&s,1,0);assert(pixels[400][10]==0xa5);
    memset(pixels,0xa5,sizeof(pixels));cd_ui_monitor(&port,&s,0,1);assert(pixels[10][10]==0xa5);
    /* Long auto/dual ISO values, unknown metadata, bulb, extreme numbers and stale battery. */
    for(int i=0;i<100;i++){
        snprintf(s.iso,sizeof(s.iso),"A12800/25600D+%d",i);snprintf(s.shutter,sizeof(s.shutter),i%2?"BULB":"1/8000");
        s.battery_bars=i%5-1;s.aperture[0]=0;cd_ui_monitor(&port,&s,1,1);
    }
    strcpy(s.iso,"400");strcpy(s.shutter,"1/125");strcpy(s.aperture,"f/5.6");s.battery_bars=2;
    monitor_only=0;
    trace=fopen("build/ui-home.jsonl","w");assert(trace);
    cd_ui_dashboard(&port,&s);fclose(trace);trace=0;frame("home");
    assert(pixels[50][91]==CD_LINE && pixels[50][218]==CD_LINE && pixels[50][303]==CD_LINE);
    assert(pixels[440][88]==CD_LINE && pixels[440][173]==CD_LINE && pixels[440][259]==CD_LINE);
    for(int i=-40;i<=40;i++){
        s.ec_eighths=i;snprintf(s.ec,sizeof(s.ec),"%s%d.%d",i<0?"-":"",abs(i)/8,abs(i)%8);
        s.drive_mode=i&3;s.metering_mode=i&7;
        strcpy(s.wb,i%2?"10000K":"SHADE");strcpy(s.iso,"A12800/25600D+");
        cd_ui_dashboard(&port,&s);
    }
    strcpy(s.wb,"AWB");strcpy(s.iso,"1000");strcpy(s.ec,"0.0");strcpy(s.ev,"--");s.ec_eighths=0;
    strcpy(s.aperture,"M.AP");cd_ui_dashboard(&port,&s);frame("manual-aperture");
    assert(pixels[0][719]==CD_BG && pixels[479][719]==CD_BG);
    instrument=0;
    for(int count=5;count<=18;count++)for(int i=0;i<count;i++)cd_ui_tab(&port,"Tab Overlay",i==0,i*(720/count),720/count);
    puts("HUI: real AA font decoder and damaged data, status-first navigation, bounds, untouched viewport, edge values and fallback gates passed.");
}
