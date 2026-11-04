/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef CAMDESK_FACE_H
#define CAMDESK_FACE_H
#include <stdint.h>
/* A validated, run-length encoded 4-bit coverage atlas. */
int cd_face_validate(const unsigned char *data,uint32_t size);
int cd_face_init(void);
int cd_face_width(int size,const char *text);
int cd_face_paint(int size,int color,int bg,int x,int y,const char *text,
    void (*fill)(int,int,int,int,int));
#endif
