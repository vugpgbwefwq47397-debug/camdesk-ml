// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef CAMDESK_ZH_TRANSLATION_H
#define CAMDESK_ZH_TRANSLATION_H
#include <stddef.h>
void zh_enable(int enabled);
const char *zh_text(const char *text);
void zh_render_copy(char *out, size_t size, const char *text);
int zh_snprintf(char *out, size_t size, const char *format, ...);
#endif
