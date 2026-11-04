"""Build a display-only Chinese overlay on the pinned upstream core."""
import ast, hashlib, json, re, shutil, subprocess
from prepare_ui import menu_overlay,lens_overlay
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
SOURCE=ROOT/'upstream/ml'
L10N=ROOT/'localization'

def cstr(value):return json.dumps(value,ensure_ascii=False)
def original(path):return subprocess.check_output(['git','-C',str(SOURCE),'show','HEAD:'+path]).decode('utf-8')
def put(path,text):
    target=SOURCE/path;target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(text,encoding='utf-8',newline='\n')
def once(text,before,after):
    if text.count(before)!=1:raise ValueError('Patch anchor changed: '+before[:80])
    return text.replace(before,after,1)

def dictionary():
    result={}
    vendor=L10N/'vendor/localization'
    for p in sorted(vendor.glob('*.json')):result.update(json.loads(p.read_text(encoding='utf-8')))
    for p in sorted(vendor.glob('*.py')):
        try:tree=ast.parse(p.read_text(encoding='utf-8'))
        except SyntaxError:continue
        for node in ast.walk(tree):
            if not isinstance(node,ast.Dict):continue
            try:data=ast.literal_eval(node)
            except (ValueError,TypeError):continue
            result.update({k:v for k,v in data.items() if isinstance(k,str) and isinstance(v,str) and any('\u3400'<=c<='\u9fff' for c in v)})
    result.update(json.loads((L10N/'overrides.json').read_text(encoding='utf-8')))
    additional=list(json.loads((L10N/'50d-additional-ui.json').read_text(encoding='utf-8')))
    translations=json.loads((L10N/'50d-additional-translations.json').read_text(encoding='utf-8'))
    if len(additional)!=362 or len(translations)!=362:raise ValueError('Additional UI audit changed')
    result.update({key:translations[str(i)] for i,key in enumerate(additional)})
    before_edit=dict(result)
    editorial=json.loads((L10N/'editorial.json').read_text(encoding='utf-8'))
    result.update(editorial['labels'])
    result.update(json.loads((ROOT/'ui/translations.json').read_text(encoding='utf-8')))
    for key,value in list(result.items()):
        for before,after in editorial['phrase_rules']:value=value.replace(before,after)
        if 'zoom' in key.lower() and not re.search(r'lens|focal',key,re.I):value=value.replace('变焦','放大').replace('缩放','放大')
        result[key]=value
    changes={key:{'before':before_edit.get(key),'after':value} for key,value in result.items() if before_edit.get(key)!=value}
    (L10N/'editorial-review.json').write_text(json.dumps(changes,ensure_ascii=False,indent=2),encoding='utf-8')
    for key,value in list(result.items()):
        if '\n' in key and len(key.split('\n'))==len(value.split('\n')):
            result.update({k:v for k,v in zip(key.split('\n'),value.split('\n')) if k.strip()})
    missing=json.loads((L10N/'ui-audit-missing.json').read_text(encoding='utf-8'))
    for key in missing:
        if key.strip() in result:result[key]=result[key.strip()]
    tokens=re.compile(r'%(?:\d+\$)?[-+ #0]*(?:\d+|\*)?(?:\.(?:\d+|\*))?(?:hh|ll|[hljztL])?[diuoxXfFeEgGaAcspn%]')
    for key,value in result.items():
        if tokens.findall(key)!=tokens.findall(value) and not re.search(r'\d+%\s*[a-zA-Z]',key):
            raise ValueError('Translation changes printf arguments: '+repr(key))
    result={k:v for k,v in result.items() if k.strip() and k!=v}
    (L10N/'zh_CN.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    return result

def translation_source(data):
    lines=['// SPDX-License-Identifier: GPL-2.0-or-later','// Generated from pinned ChaseChanCN translations plus CamDesk overrides.','#ifdef ZH_HOST_TEST','#include <stdio.h>','#include <stdarg.h>','#include <string.h>','#else','#include "dryos.h"','#endif','#include "zh_translation.h"','static int chinese_enabled;','void zh_enable(int enabled) { chinese_enabled=enabled; }']
    entries=sorted(data.items(),key=lambda item:item[0].encode('utf-8'))
    for i,(_,value) in enumerate(entries):lines.append(f'static char value_{i}[] = {cstr(value)};')
    lines+=['static const struct { const char *english; char *chinese; unsigned short length; } translations[] = {']
    lines += [f'    {{{cstr(key)}, value_{i}, {len(key.encode("utf-8"))}}},' for i,(key,_) in enumerate(entries)]
    lines+=['};']
    for label,end in [('first',False),('past',True)]:
        offsets=[]
        for byte in range(256):
            indices=[i for i,(key,_) in enumerate(entries) if key.encode('utf-8')[0]==byte]
            offsets.append((indices[-1]+1 if end else indices[0]) if indices else 0)
        lines.append('static const unsigned short '+label+'[256] = {'+','.join(map(str,offsets))+'};')
    lines+=[r'''
const char *zh_text(const char *text)
{
    if (!chinese_enabled || !text || !*text) return text;
    int lo=0,hi=(int)(sizeof(translations)/sizeof(translations[0]))-1;
    while(lo<=hi) {
        int mid=(lo+hi)/2,cmp=strcmp(text,translations[mid].english);
        if(!cmp)return translations[mid].chinese;
        if(cmp>0)lo=mid+1;else hi=mid-1;
    }
    return text;
}
static void trim_utf8(char *text)
{
    unsigned char *p=(unsigned char *)text;
    while(*p) {
        int n=*p>=0xf0&&*p<=0xf4?4:*p>=0xe0&&*p<=0xef?3:*p>=0xc2&&*p<=0xdf?2:1;
        for(int i=1;i<n;i++)if(!p[i]||(p[i]&0xc0)!=0x80){*p=0;return;}
        p+=n;
    }
}
static int word(unsigned char c) { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'; }
void zh_render_copy(char *out,size_t size,const char *text)
{
    if(!size)return;
    const char *exact=zh_text(text);
    if(exact!=text || !chinese_enabled || strchr(text,'/') || strchr(text,'\\')) {
        snprintf(out,size,"%s",exact);trim_utf8(out);return;
    }
    size_t used=0;const char *p=text;
    while(*p && used+1<size) {
        int found=-1,length=0;
        unsigned char initial=(unsigned char)*p;
        if(p==text || !word((unsigned char)p[-1])) {
            for(int i=first[initial];i<past[initial];i++) {
                int n=translations[i].length;
                if(n<=length || strncmp(p,translations[i].english,n))continue;
                if(word((unsigned char)translations[i].english[n-1]) && word((unsigned char)p[n]))continue;
                found=i;length=n;
            }
        }
        const char *chunk=found<0?p:translations[found].chinese;
        size_t count=found<0?1:strlen(chunk);
        if(count>size-used-1)count=size-used-1;
        memcpy(out+used,chunk,count);used+=count;p+=found<0?1:length;
    }
    out[used]=0;trim_utf8(out);
}
int zh_snprintf(char *out,size_t size,const char *format,...)
{
    va_list args;va_start(args,format);
    int result=vsnprintf(out,size,zh_text(format),args);
    va_end(args);
    if(size) {
        char localized[256];
        zh_render_copy(localized,sizeof(localized),out);
        result=snprintf(out,size,"%s",localized);
        trim_utf8(out);
    }
    return result;
}
''']
    return '\n'.join(lines)

def main():
    state_file=L10N/'overlay-state.json'
    previous=json.loads(state_file.read_text()) if state_file.exists() else {}
    changed=['src/rbf_font.c','src/bmp.c','src/notify_box.c','src/menu.c','src/menu.h','src/lens.c','platform/Makefile']
    for path in changed:
        current=(SOURCE/path).read_text(encoding='utf-8')
        digest=hashlib.sha256(current.encode()).hexdigest()
        if current!=original(path) and previous.get(path)!=digest:raise SystemExit('Unexpected local edit: '+path)
    data=dictionary()
    put('src/zh_translation.c',translation_source(data))
    put('src/zh_translation.h',(L10N/'zh_translation.h').read_text())
    put('src/cjk_font.c',(L10N/'cjk_font.c').read_text())
    header=(L10N/'vendor/src/cjk_font.h').read_text()
    header=header.replace('#endif','int cjk_font_validate(const char *data, uint32_t size, int expected_height);\n#endif')
    put('src/cjk_font.h',header)
    renderer=(L10N/'vendor/src/rbf_font.c').read_text()
    renderer='#include "zh_translation.h"\n'+renderer
    renderer=once(renderer,'    cjk_fonts_init();','    zh_enable(cjk_fonts_init() > 0);')
    font_anchor='    font_by_name("argnor32", COLOR_BLACK, COLOR_WHITE);'
    if renderer.count(font_anchor)!=2:raise ValueError('Native font initialization changed')
    renderer=renderer.replace(font_anchor,font_anchor+'\n    font_by_name("cdnord40", COLOR_BLACK, COLOR_WHITE);',1)
    put('src/rbf_font.c',renderer)
    bmp=original('src/bmp.c');bmp='#include "zh_translation.h"\n'+bmp
    bmp=once(bmp,'*x, *y, s, FONT(fontspec, fg_color, bg_color));','*x, *y, zh_text(s), FONT(fontspec, fg_color, bg_color));')
    bmp=bmp.replace('sizeof(bmp_printf_buf)-1, fmt, ap','sizeof(bmp_printf_buf)-1, zh_text(fmt), ap')
    bmp=once(bmp,'    return bmp_puts( fontspec, &x, &y, bmp_printf_buf );','    char localized[128];\n    zh_render_copy(localized,sizeof(localized),bmp_printf_buf);\n    return bmp_puts( fontspec, &x, &y, localized );')
    bmp=once(bmp,'        ans = bmp_puts( fontspec, &x, &y, bmp_printf_buf );','        static char localized[1024];\n        zh_render_copy(localized,sizeof(localized),bmp_printf_buf);\n        ans = bmp_puts( fontspec, &x, &y, localized );')
    bmp=once(bmp,'font_dynamic[FONT_ID(fontspec)].bitmap, str);','font_dynamic[FONT_ID(fontspec)].bitmap, zh_text(str));')
    put('src/bmp.c',bmp)
    notify=original('src/notify_box.c');notify='#include "zh_translation.h"\n'+notify
    notify=once(notify,'sizeof(notify_box_msg_tmp)-1, fmt, ap','sizeof(notify_box_msg_tmp)-1, zh_text(fmt), ap')
    put('src/notify_box.c',notify)
    menu=original('src/menu.c');menu='#include "zh_translation.h"\n'+menu
    menu=once(menu,'static void print_help_line(int color, int x, int y, char* msg)\n{','static void print_help_line(int color, int x, int y, char* msg)\n{\n    msg = (char*)zh_text(msg);')
    menu=menu.replace('pickbox_string(entry, i), i < entry->max','zh_text(pickbox_string(entry, i)), i < entry->max')
    put('src/menu.c',menu_overlay(menu))
    put('src/lens.c',lens_overlay(original('src/lens.c')))
    for target,source in [('nordic.h','nordic.h'),('nordic_draw.c','nordic_draw.c'),('camdesk_ui.c','integration.c'),('face.h','face.h'),('face.c','face.c')]:
        put('src/'+target,(ROOT/'ui'/source).read_text(encoding='utf-8'))
    header=original('src/menu.h');header=once(header,'#define _menu_h_','#define _menu_h_\n#include "zh_translation.h"')
    # Only UI format macros use translated formatting. Internal names and lookups remain intact.
    header=header.replace('snprintf(info->','zh_snprintf(info->')
    put('src/menu.h',header)
    makefile=original('platform/Makefile')
    makefile=once(makefile,'    rbf_font.o \\', '    rbf_font.o \\\n    cjk_font.o \\\n    zh_translation.o \\')
    makefile=once(makefile,'    zh_translation.o \\', '    zh_translation.o \\\n    nordic_draw.o \\\n    camdesk_ui.o \\\n    face.o \\')
    put('platform/Makefile',makefile)
    state_file.write_text(json.dumps({p:hashlib.sha256((SOURCE/p).read_bytes()).hexdigest() for p in changed},indent=2))
    print('Chinese overlay ready:',len(data),'translation entries; original menu IDs preserved')

if __name__=='__main__':main()
