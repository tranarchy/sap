#include <string.h>
#include <SDL2/SDL.h>

#include <microui.h>

#include "atlas.h"

#define BUFFER_SIZE 16384

static float         tex_buf[BUFFER_SIZE *  8];
static float         vert_buf[BUFFER_SIZE *  8];
static unsigned char color_buf[BUFFER_SIZE * 16];
static int           index_buf[BUFFER_SIZE *  6];

static SDL_Window *window;

static SDL_Texture *texture;
static SDL_Renderer *renderer;

static int height, width;

static int buf_idx;

void sdlr_init(void) {
   window = SDL_CreateWindow(
    "sap", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    800, 700, SDL_WINDOW_SHOWN);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, ATLAS_WIDTH, ATLAS_HEIGHT);

  unsigned char rgba_texture[ATLAS_WIDTH * ATLAS_HEIGHT * 4];
  unsigned char byte_texture[ATLAS_HEIGHT][ATLAS_WIDTH];

	for(int i = 0; i < ATLAS_HEIGHT; i++) {
    for(int j = 0; j < ATLAS_WIDTH; j++) {
      byte_texture[i][j] = 0;
    }
  }

  unsigned char r, g, b, a;
	int rgba_index = 0;

	for(int i = 0; i < ATLAS_HEIGHT; i++) {
    for(int j = 0; j < ATLAS_WIDTH; j++) {
    
      char current_char = atlas_texture[i][j];

      if (current_char == '*') {
        byte_texture[i][j] = 1;
                   
        for (int dy = -1; dy <= 1; dy++) {
          for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;

            int ny = i + dy;
            int nx = j + dx;

            if (ny >= 0 && nx >= 0 && ny < ATLAS_HEIGHT && nx < ATLAS_WIDTH) {
              if (byte_texture[ny][nx] == 0) {
                byte_texture[ny][nx] = 2;
              }
            }
          }
        }
      }
    }
  }

  for(int i = 0; i < ATLAS_HEIGHT; i++) {
    for(int j = 0; j < ATLAS_WIDTH; j++) {
      unsigned char atlas_cur = byte_texture[i][j];

			if (atlas_cur == 1) {
				r = 255;
				g = 255;
				b = 255;
				a = 255;
			} else if (atlas_cur == 2) {
				r = 0;
				g = 0;
				b = 0;
				a = 255;
			} else {
        r = 0;
				g = 0;
				b = 0;
				a = 0;
			}

      rgba_texture[rgba_index++] = r;
      rgba_texture[rgba_index++] = g;
      rgba_texture[rgba_index++] = b;
      rgba_texture[rgba_index++] = a;
    }
  }

  SDL_UpdateTexture(texture, NULL, rgba_texture, 4 * ATLAS_WIDTH);
  SDL_RenderSetVSync(renderer, 1);

  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
 
}

void sdlr_flush(void) {
  if (buf_idx == 0) { return; }

  SDL_GetWindowSize(window, &width, &height);
  SDL_Rect viewport = {0, 0, width, height};
  SDL_RenderSetViewport(renderer, &viewport);

  int xy_stride = 2 * sizeof(float);
  float *xy = vert_buf;

  int uv_stride = 2 * sizeof(float);
  float *uv = tex_buf;

  int col_stride = 4 * sizeof(uint8_t);
  SDL_Color *color = (SDL_Color*)color_buf;

  SDL_RenderGeometryRaw(renderer, texture,
          xy, xy_stride, color,
          col_stride,
          uv, uv_stride,
          buf_idx * 4,
          index_buf, buf_idx * 6, sizeof (int));

  SDL_RenderPresent(renderer);

  buf_idx = 0;
}

static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
  if (buf_idx == BUFFER_SIZE) { sdlr_flush(); }
  
  int texvert_idx = buf_idx *  8;
  int   color_idx = buf_idx * 4 * sizeof(mu_Color);
  int element_idx = buf_idx *  4;
  int   index_idx = buf_idx *  6;
  buf_idx++;

  /* update texture buffer */
  float x = src.x / (float) ATLAS_WIDTH;
  float y = src.y / (float) ATLAS_HEIGHT;
  float w = src.w / (float) ATLAS_WIDTH;
  float h = src.h / (float) ATLAS_HEIGHT;
  tex_buf[texvert_idx + 0] = x;
  tex_buf[texvert_idx + 1] = y;
  tex_buf[texvert_idx + 2] = x + w;
  tex_buf[texvert_idx + 3] = y;
  tex_buf[texvert_idx + 4] = x;
  tex_buf[texvert_idx + 5] = y + h;
  tex_buf[texvert_idx + 6] = x + w;
  tex_buf[texvert_idx + 7] = y + h;

  /* update vertex buffer */
  vert_buf[texvert_idx + 0] = dst.x;
  vert_buf[texvert_idx + 1] = dst.y;
  vert_buf[texvert_idx + 2] = dst.x + dst.w;
  vert_buf[texvert_idx + 3] = dst.y;
  vert_buf[texvert_idx + 4] = dst.x;
  vert_buf[texvert_idx + 5] = dst.y + dst.h;
  vert_buf[texvert_idx + 6] = dst.x + dst.w;
  vert_buf[texvert_idx + 7] = dst.y + dst.h;

  /* update color buffer */
  memcpy(color_buf + (color_idx + 0 * sizeof(mu_Color)), &color, sizeof(mu_Color));
  memcpy(color_buf + (color_idx + 1 * sizeof(mu_Color)), &color, sizeof(mu_Color));
  memcpy(color_buf + (color_idx + 2 * sizeof(mu_Color)), &color, sizeof(mu_Color));
  memcpy(color_buf + (color_idx + 3 * sizeof(mu_Color)), &color, sizeof(mu_Color));

  /* update index buffer */
  index_buf[index_idx++] = element_idx + 0;
  index_buf[index_idx++] = element_idx + 1;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 3;
  index_buf[index_idx++] = element_idx + 1;
}

int sdlr_get_text_width(const char *text, int len) {
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    res += atlas[ATLAS_FONT + chr].w;
  }
  return res;
}

int sdlr_get_text_height(void) {
  return 18;
}

void sdlr_draw_rect(mu_Rect rect, mu_Color color) {
  push_quad(rect, atlas[ATLAS_WHITE], color);
}

void sdlr_draw_icon(int id, mu_Rect rect, mu_Color color) {
  mu_Rect src = atlas[id];
  int x = rect.x + (rect.w - src.w) / 2;
  int y = rect.y + (rect.h - src.h) / 2;
  push_quad(mu_rect(x, y, src.w, src.h), src, color);
}

void sdlr_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
  mu_Rect dst = { pos.x, pos.y, 0, 0 };
  for (const char *p = text; *p; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    mu_Rect src = atlas[ATLAS_FONT + chr];
    dst.w = src.w;
    dst.h = src.h;
    push_quad(dst, src, color);
    dst.x += dst.w;
  }
}

void sdlr_set_clip_rect(mu_Rect rect) {
  SDL_Rect clip_rect = { rect.x, height - (rect.y + rect.h), rect.w, rect.h };
  SDL_RenderSetClipRect(renderer, &clip_rect);
}

void sdlr_clear(mu_Color clr) {
  SDL_SetRenderDrawColor(renderer, clr.r, clr.g, clr.b, clr.a);
  SDL_RenderClear(renderer);
}

void sdlr_present(void) {
  sdlr_flush();
}
