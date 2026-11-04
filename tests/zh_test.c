#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cjk_font.h"
#include "zh_translation.h"
int main(void)
{
    int step;
    assert(utf8_decode("拍",&step)==0x62cd && step==3);
    assert(utf8_decode("A",&step)=='A' && step==1);
    const char *bad[]={"\xe6", "\xe6\x8b", "\xc0\xaf", "\xed\xa0\x80", "\xf4\x90\x80\x80"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
        assert(utf8_decode(bad[i],&step)==(unsigned char)bad[i][0] && step==1);
    }
    FILE *file=fopen("localization/fonts/cjk32.rbf","rb");assert(file);
    fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);
    char *font=malloc(size);assert(font);assert(fread(font,1,size,file)==(size_t)size);fclose(file);
    assert(cjk_font_validate(font,(uint32_t)size,32));
    for(unsigned i=0;i<100;i++)assert(!cjk_font_validate(font,i,32));
    assert(!cjk_font_validate(font,(uint32_t)size-1,32));
    assert(!cjk_font_validate(font,(uint32_t)size,23));
    char saved=font[4];font[4]=2;assert(!cjk_font_validate(font,(uint32_t)size,32));font[4]=saved;
    saved=font[18];font[18]=0;assert(!cjk_font_validate(font,(uint32_t)size,32));font[18]=saved;
    free(font);
    assert(cjk_fonts_init()>1000);
    const char *text="拍摄间隔开始延迟次数停止设置模块工作台";
    for(const char *p=text;*p;){
        uint32_t cp=utf8_decode(p,&step);
        assert(cjk_has(cp,12)&&cjk_has(cp,20)&&cjk_has(cp,23)&&cjk_has(cp,28)&&cjk_has(cp,32)&&cjk_has(cp,40));p+=step;
    }
    assert(!strcmp(zh_text("Modules"),"Modules"));
    zh_enable(1);
    assert(!strcmp(zh_text("Modules"),"模块"));
    assert(!strcmp(zh_text("Arm plan"),"准备拍摄"));
    assert(!strcmp(zh_text("ML/SETTINGS/camdesk.en"),"ML/SETTINGS/camdesk.en"));
    char buffer[128];zh_snprintf(buffer,sizeof(buffer),"Interval time too short (%ds)",5);
    assert(!strcmp(buffer,"拍摄间隔过短（5秒）"));
    zh_snprintf(buffer,5,"Modules");assert(!strcmp(buffer,"模"));
    zh_snprintf(buffer,sizeof(buffer),"%s. Press %s for more info.","Module loaded successfully","FUNC");
    assert(strstr(buffer,"模块加载成功") && strstr(buffer,"查看详情"));
    zh_render_copy(buffer,sizeof(buffer),"ML/SETTINGS/camdesk.en");
    assert(!strcmp(buffer,"ML/SETTINGS/camdesk.en"));
    puts("Chinese: UTF-8, malformed fonts, all six font sizes, identifiers and format arguments passed");
    return 0;
}
