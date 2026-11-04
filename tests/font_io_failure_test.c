#include <assert.h>
#include <stdio.h>
#include "face.h"
#include "cjk_font.h"
int main(void)
{
    assert(!cd_face_init());
    assert(!cjk_fonts_init());
    assert(cd_face_width(40,"400")==-1);
    puts("Font I/O failure: failed reads leave fonts unavailable without crashing.");
}
