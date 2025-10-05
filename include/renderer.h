#ifndef RENDERER_H
#define RENDERER_H

#include <microui.h>

void sdlr_init(void);
void sdlr_draw_rect(mu_Rect rect, mu_Color color);
void sdlr_draw_text(const char *text, mu_Vec2 pos, mu_Color color);
void sdlr_draw_icon(int id, mu_Rect rect, mu_Color color);
 int sdlr_get_text_width(const char *text, int len);
 int sdlr_get_text_height(void);
void sdlr_set_clip_rect(mu_Rect rect);
void sdlr_clear(mu_Color color);
void sdlr_present(void);

#endif
