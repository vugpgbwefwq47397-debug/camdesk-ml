/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifdef CJK_HOST_TEST
#include "cjk_host.h"
#define CD_FACE_PATH "localization/fonts/cdneutral.cdf"
#else
#include "dryos.h"
#define CD_FACE_PATH "ML/FONTS/CDNEUTRAL.CDF"
#endif
#include "face.h"

#define FACE_MAX_BYTES (1024*1024)
#define FACE_CHARS 95
#define FACE_SIZES 11
#define FACE_COUNT (FACE_CHARS*FACE_SIZES)
static const int heights[FACE_SIZES]={12,20,23,28,32,40,56,64,96,128,160};
struct glyph {uint32_t offset,length;uint16_t width,advance;};
static struct glyph glyphs[FACE_COUNT];
static unsigned char *atlas;
static int initialized,ready;
static uint16_t u16(const unsigned char *p){uint16_t v;memcpy(&v,p,2);return v;}
static uint32_t u32(const unsigned char *p){uint32_t v;memcpy(&v,p,4);return v;}

static int parse(const unsigned char *data,uint32_t size,int install)
{
    if(!data || size<16 || size>FACE_MAX_BYTES || memcmp(data,"CDF2",4) ||
       u32(data+4)!=1 || u32(data+8)!=FACE_COUNT || u32(data+12)!=size)return 0;
    uint32_t pos=16;
    for(int i=0;i<FACE_COUNT;i++){
        if(size-pos<12)return 0;
        uint16_t height=u16(data+pos),cp=u16(data+pos+2),w=u16(data+pos+4),advance=u16(data+pos+6);
        uint32_t length=u32(data+pos+8);pos+=12;
        if(height!=heights[i/FACE_CHARS] || cp!=32+i%FACE_CHARS ||
           !w || w>256 || advance<w || advance>256 || !length || length>size-pos)return 0;
        uint32_t expected=(uint32_t)w*height,total=0;
        for(uint32_t j=0;j<length;j++){
            uint32_t run=(data[pos+j]&15)+1;
            if(run>expected-total)return 0;
            total+=run;
        }
        if(total!=expected)return 0;
        if(install)glyphs[i]=(struct glyph){pos,length,w,advance};
        pos+=length;
    }
    return pos==size;
}
int cd_face_validate(const unsigned char *data,uint32_t size){return parse(data,size,0);}
int cd_face_init(void)
{
    if(initialized)return ready;
    initialized=1;
    uint32_t size=0;
    if(FIO_GetFileSize(CD_FACE_PATH,&size) || size<16 || size>FACE_MAX_BYTES)return 0;
    FILE *file=FIO_OpenFile(CD_FACE_PATH,O_RDONLY|O_SYNC);if(!file)return 0;
    unsigned char *data=malloc(size);
    if(!data){FIO_CloseFile(file);return 0;}
    /* FIO requires uncached DMA memory; a cached read over 8192 asserts on camera. */
    char *io=fio_malloc(8192);
    uint32_t offset=0;
    if(io){
        while(offset<size){
            int count=FIO_ReadFile(file,io,MIN(8192,size-offset));
            if(count<=0 || (uint32_t)count>MIN(8192,size-offset))break;
            memcpy(data+offset,io,count);offset+=count;
        }
        fio_free(io);
    }
    FIO_CloseFile(file);
    if(offset!=size || !parse(data,size,1)){free(data);return 0;}
    atlas=data;ready=1;return 1;
}
static int group(int size)
{
    for(int i=0;i<FACE_SIZES;i++)if(heights[i]==size)return i;
    return -1;
}
int cd_face_width(int size,const char *text)
{
    int g=group(size);if(!ready || g<0 || !text)return -1;
    int width=0;
    for(const unsigned char *p=(const unsigned char*)text;*p;p++){
        if(*p<32 || *p>126)return -1;
        int advance=glyphs[g*FACE_CHARS+*p-32].advance;
        if(width>4096-advance)return -1;
        width+=advance;
    }
    return width;
}
static int shade(int a,int fg,int bg)
{
    if(a==0)return bg;
    if(a==15)return fg;
    /* Native grayscale palette; other accent colors use a crisp threshold. */
    if(fg==79 && (bg==2 || bg==40))return 38+(a*41+7)/15;
    return a>=8?fg:bg;
}
int cd_face_paint(int size,int color,int bg,int x,int y,const char *text,
    void (*fill)(int,int,int,int,int))
{
    int width=cd_face_width(size,text),g=group(size);
    if(width<0 || !fill || x<0 || y<0 || x+width>720 || y+size>480)return 0;
    for(const unsigned char *p=(const unsigned char*)text;*p;p++){
        const struct glyph *glyph=&glyphs[g*FACE_CHARS+*p-32];
        uint32_t position=0;
        for(uint32_t i=0;i<glyph->length;i++){
            int token=atlas[glyph->offset+i],run=(token&15)+1,c=shade(token>>4,color,bg);
            while(run){
                int gx=position%glyph->width,gy=position/glyph->width;
                int n=MIN(run,glyph->width-gx);
                fill(c,x+gx,y+gy,n,1);position+=n;run-=n;
            }
        }
        if(glyph->advance>glyph->width)fill(bg,x+glyph->width,y,glyph->advance-glyph->width,size);
        x+=glyph->advance;
    }
    return 1;
}
