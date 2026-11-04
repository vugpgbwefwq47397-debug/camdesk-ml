// SPDX-License-Identifier: GPL-2.0-or-later
// CRBF format and rendering integration derived from ChaseChanCN/magiclantern.
#ifdef CJK_HOST_TEST
#include "cjk_host.h"
#define CJK_FONT_DIR "localization/fonts"
#else
#include "dryos.h"
#include "bmp.h"
#endif
#include "cjk_font.h"

#ifndef CJK_FONT_DIR
#define CJK_FONT_DIR "ML/FONTS"
#endif
#define CJK_NSIZES 6
static const int heights[CJK_NSIZES] = {12, 20, 23, 28, 32, 40};
typedef struct { uint32_t cp, offset; uint16_t width, advance; } glyph_t;
typedef struct { int height, count; glyph_t *glyphs; char *data; } font_t;
static font_t fonts[CJK_NSIZES];
static uint16_t rd16(const char *p) { uint16_t n; memcpy(&n,p,2); return n; }
static uint32_t rd32(const char *p) { uint32_t n; memcpy(&n,p,4); return n; }

int cjk_font_validate(const char *data, uint32_t size, int expected_height)
{
    if (!data || size < 14 || size > 1024*1024 || memcmp(data,"CRBF",4) || rd32(data+4)!=1) return 0;
    uint32_t count=rd32(data+10), offset=14, previous=0;
    int height=rd16(data+8);
    if (height!=expected_height || height<8 || height>48 || !count || count>10000) return 0;
    for (uint32_t i=0;i<count;i++) {
        if (offset>size || size-offset<8) return 0;
        uint32_t cp=rd32(data+offset);
        uint32_t width=rd16(data+offset+4), advance=rd16(data+offset+6);
        if (cp<0x80 || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff) || cp<=previous || !width || width>64 || !advance || advance>65) return 0;
        uint32_t bytes=((width+7)/8)*height;
        offset+=8;
        if (bytes>size-offset) return 0;
        offset+=bytes; previous=cp;
    }
    return offset==size;
}

static int load_one(const char *name, font_t *font, int height)
{
    uint32_t size=0;
    if (FIO_GetFileSize(name,&size) || size<14 || size>1024*1024) return 0;
    FILE *file=FIO_OpenFile(name,O_RDONLY|O_SYNC);
    if (!file) return 0;
    char *data=malloc(size);
    if (!data) { FIO_CloseFile(file); return 0; }
    char *io=fio_malloc(8192);
    uint32_t loaded=0;
    if(io) {
        while(loaded<size) {
            uint32_t chunk=MIN(8192,size-loaded);
            int count=FIO_ReadFile(file,io,chunk);
            if(count<=0 || (uint32_t)count>chunk)break;
            memcpy(data+loaded,io,count);loaded+=count;
        }
        fio_free(io);
    }
    FIO_CloseFile(file);
    if(loaded!=size) { free(data); return 0; }
    if (!cjk_font_validate(data,size,height)) { free(data); return 0; }
    uint32_t count=rd32(data+10), offset=14;
    glyph_t *glyphs=malloc(count*sizeof(glyph_t));
    if (!glyphs) { free(data); return 0; }
    for (uint32_t i=0;i<count;i++) {
        glyphs[i].cp=rd32(data+offset);
        glyphs[i].width=rd16(data+offset+4);
        glyphs[i].advance=rd16(data+offset+6);
        glyphs[i].offset=offset+8;
        offset+=8+((glyphs[i].width+7)/8)*height;
    }
    font->height=height; font->count=count; font->glyphs=glyphs; font->data=data;
    return count;
}

int cjk_fonts_init(void)
{
    static int initialized=0, total=0;
    if (initialized) return total;
    initialized=1;
    int loaded_total=0;
    for (int i=0;i<CJK_NSIZES;i++) {
        char name[64];
        snprintf(name,sizeof(name),CJK_FONT_DIR "/cjk%d.rbf",heights[i]);
        int count=load_one(name,&fonts[i],heights[i]);
        if (!count) return 0; /* Disable Chinese unless every size is available. */
        loaded_total+=count;
    }
    total=loaded_total;
    return total;
}

uint32_t utf8_decode(const char *s, int *advance)
{
    const unsigned char *p=(const unsigned char *)s;
    uint32_t cp=p[0], minimum=0;
    *advance=1;
    if (cp<0x80) return cp;
    int n;
    if (cp>=0xc2 && cp<=0xdf) { n=2; cp&=0x1f; minimum=0x80; }
    else if (cp>=0xe0 && cp<=0xef) { n=3; cp&=0xf; minimum=0x800; }
    else if (cp>=0xf0 && cp<=0xf4) { n=4; cp&=7; minimum=0x10000; }
    else return p[0];
    for (int i=1;i<n;i++) {
        if (!p[i] || (p[i]&0xc0)!=0x80) return p[0];
        cp=(cp<<6)|(p[i]&0x3f);
    }
    if (cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff)) return p[0];
    *advance=n;
    return cp;
}

static font_t *pick_font(int height)
{
    font_t *best=0; int distance=1000;
    for (int i=0;i<CJK_NSIZES;i++) {
        int d=heights[i]-height; if(d<0)d=-d;
        if (fonts[i].data && d<distance) { best=&fonts[i]; distance=d; }
    }
    return best;
}

int cjk_glyph(uint32_t cp,int height,const char **data,int *cellwidth,int *width,int *glyphheight)
{
    font_t *font=pick_font(height);
    if (!font) return 0;
    int lo=0,hi=font->count-1;
    while(lo<=hi) {
        int mid=(lo+hi)/2; glyph_t *g=&font->glyphs[mid];
        if(g->cp==cp) {
            *data=font->data+g->offset; *cellwidth=(g->width+7)&~7;
            *width=g->width; *glyphheight=font->height; return g->advance;
        }
        if(g->cp<cp)lo=mid+1; else hi=mid-1;
    }
    return 0;
}

int cjk_has(uint32_t cp,int height)
{
    const char *data; int a,b,c;
    return cjk_glyph(cp,height,&data,&a,&b,&c)!=0;
}
