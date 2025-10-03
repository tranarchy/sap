#ifndef RENDERER_H
#define RENDERER_H

#include <microui.h>

void gl_init(void);
void gl_draw_rect(mu_Rect rect, mu_Color color);
void gl_draw_text(const char *text, mu_Vec2 pos, mu_Color color);
void gl_draw_icon(int id, mu_Rect rect, mu_Color color);
 int gl_get_text_width(const char *text, int len);
 int gl_get_text_height(void);
void gl_set_clip_rect(mu_Rect rect);
void gl_clear(mu_Color color);
void gl_present(void);

#endif
