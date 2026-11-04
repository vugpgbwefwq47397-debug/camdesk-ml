#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#define MIN(a,b) ((a)<(b)?(a):(b))
static int FIO_GetFileSize(const char *path,uint32_t *size)
{
    FILE *f=fopen(path,"rb");if(!f)return 1;
    fseek(f,0,SEEK_END);long n=ftell(f);fclose(f);
    if(n<0)return 1;*size=(uint32_t)n;return 0;
}
static FILE *FIO_OpenFile(const char *path,int flags) { (void)flags;return fopen(path,"rb"); }
static void *dma_buffer;
static void *fio_malloc(size_t size) {
    assert(!dma_buffer);
    if(getenv("CD_TEST_IO_ALLOC"))return NULL;
    dma_buffer=malloc(size);return dma_buffer;
}
static void fio_free(void *data) {assert(data==dma_buffer);free(data);dma_buffer=NULL;}
static int FIO_ReadFile(FILE *file,char *data,uint32_t size) {
    /* Model the real DMA restriction and partial reads, rather than permissive stdio. */
    assert(data==dma_buffer && size<=8192);
    if(getenv("CD_TEST_IO_EOF"))return 0;
    if(getenv("CD_TEST_IO_ERROR"))return -1;
    return (int)fread(data,1,MIN(size,997),file);
}
static void FIO_CloseFile(FILE *file) {fclose(file);}
