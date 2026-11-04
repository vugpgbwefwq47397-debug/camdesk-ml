/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Included in lens.c after its native formatting callbacks. */
static void cd_copy_value(char *out,unsigned size,const char *in)
{
    unsigned n=0;
    if(!size)return;
    while(in && *in && n+1<size){
        unsigned char c=(unsigned char)*in++;
        const char *replacement=c==0x80?"":c==0x81?"f/":c==0x82?"1/":0;
        if(replacement){while(*replacement && n+1<size)out[n++]=*replacement++;}
        else out[n++]=(char)c;
    }
    out[n]=0;
}
static void cd_decimal(char *out,unsigned size,int tenths)
{
    int value=tenths<0?-tenths:tenths;
    snprintf(out,size,"%s%d.%d",tenths<0?"-":"",value/10,value%10);
}
void camdesk_ui_snapshot(struct cd_ui_snapshot *s)
{
    memset(s,0,sizeof(*s));
    struct lvinfo_item item={0};
    mode_update(&item,0);cd_copy_value(s->mode,sizeof(s->mode),item.value);
    tv_update(&item,0);cd_copy_value(s->shutter,sizeof(s->shutter),item.value);
    av_update(&item,0);cd_copy_value(s->aperture,sizeof(s->aperture),item.value);
    if(!s->aperture[0])snprintf(s->aperture,sizeof(s->aperture),"M.AP");
    iso_update(&item,0);cd_copy_value(s->iso,sizeof(s->iso),item.value);
    picq_update(&item,0);cd_copy_value(s->quality,sizeof(s->quality),item.value);
    wb_update(&item,0);cd_copy_value(s->wb,sizeof(s->wb),item.value);
    if(streq(s->iso,"Auto"))snprintf(s->iso,sizeof(s->iso),"A");
    if(streq(s->mode,"[]") || streq(s->mode,"Auto"))snprintf(s->mode,sizeof(s->mode),"A");
    int wb_mode=lv?lens_info.wb_mode:camdesk_wb_mode_ph;
    if(streq(s->wb,"UniWB"))snprintf(s->wb,sizeof(s->wb),"UNI");
    else if(wb_mode==WB_KELVIN)snprintf(s->wb,sizeof(s->wb),"%dK",lv?lens_info.kelvin:wb_kelvin_ph);
    else snprintf(s->wb,sizeof(s->wb),"%s",wb_mode==0?"AWB":wb_mode==1?"DAY":
        wb_mode==2?"CLD":wb_mode==3?"TUNG":wb_mode==4?"FL":
        wb_mode==5?"FLASH":wb_mode==6?"CUST":wb_mode==8?"SHADE":"WB?");
    snprintf(s->focus,sizeof(s->focus),"%s",is_manual_focus()?"MF":"AF");
    if(get_shooting_card()){
        free_space_update(&item,0);
        snprintf(s->card,sizeof(s->card),"CF %s",item.value?item.value:"--");
    }else snprintf(s->card,sizeof(s->card),"CF --");
    s->battery_bars=battery_level_bars;
    s->peaking=is_focus_peaking_enabled();
    s->ec_eighths=lens_info.ae;
    int magnitude=s->ec_eighths<0?-s->ec_eighths:s->ec_eighths;
    cd_decimal(s->ec,sizeof(s->ec),(s->ec_eighths<0?-1:1)*((magnitude*10+4)/8));
    /* Photographic EV = log2(N^2/t); unknown lens/time must not invent a reading. */
    if(lens_info.raw_shutter && lens_info.raw_aperture && s->aperture[0]!='M' &&
       s->shutter[0] && !streq(s->shutter,"BULB"))
        cd_decimal(s->ev,sizeof(s->ev),APEX10_RAW2TV(lens_info.raw_shutter)+APEX10_RAW2AV(lens_info.raw_aperture));
    else snprintf(s->ev,sizeof(s->ev),"--");
    s->drive_mode=drive_mode;
    s->metering_mode=metering_mode;
}
