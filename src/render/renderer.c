#include <string.h>
#include <SDL2/SDL.h>

#include <glad.h>
#include <microui.h>

#include "atlas.h"

#define BUFFER_SIZE 16384

static GLfloat   tex_buf[BUFFER_SIZE *  8];
static GLfloat  vert_buf[BUFFER_SIZE *  8];
static GLubyte color_buf[BUFFER_SIZE * 16];
static GLuint  index_buf[BUFFER_SIZE *  6];

static SDL_Window *window;

static int height, width;

static int buf_idx;

void gl_init(void) {
   window = SDL_CreateWindow(
    "sap", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    800, 700, SDL_WINDOW_OPENGL);
  SDL_GL_CreateContext(window);

  gladLoadGL();

  /* init gl */
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glEnableClientState(GL_COLOR_ARRAY);

  GLubyte byte_texture[ATLAS_HEIGHT][ATLAS_WIDTH];

  GLubyte rgba_texture[ATLAS_WIDTH * ATLAS_HEIGHT * 4];

	for(int i = 0; i < ATLAS_HEIGHT; i++) {
    for(int j = 0; j < ATLAS_WIDTH; j++) {
      byte_texture[i][j] = 0;
    }
  }

  GLuint r, g, b, a;
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
      GLubyte atlas_cur = byte_texture[i][j];

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

  GLuint id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
    GL_RGBA, GL_UNSIGNED_BYTE, rgba_texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void gl_flush(void) {
  if (buf_idx == 0) { return; }

  SDL_GetWindowSize(window, &width, &height);

  glViewport(0, 0, width, height);

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glTexCoordPointer(2, GL_FLOAT, 0, tex_buf);
  glVertexPointer(2, GL_FLOAT, 0, vert_buf);
  glColorPointer(4, GL_UNSIGNED_BYTE, 0, color_buf);

  glDrawElements(GL_TRIANGLES, buf_idx * 6, GL_UNSIGNED_INT, index_buf);

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  buf_idx = 0;
}

static void push_quad(mu_Rect dst, mu_Rect src, mu_Color color) {
  if (buf_idx == BUFFER_SIZE) { gl_flush(); }
  
  int texvert_idx = buf_idx *  8;
  int   color_idx = buf_idx * 16;
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
  memcpy(color_buf + color_idx +  0, &color, 4);
  memcpy(color_buf + color_idx +  4, &color, 4);
  memcpy(color_buf + color_idx +  8, &color, 4);
  memcpy(color_buf + color_idx + 12, &color, 4);

  /* update index buffer */
  index_buf[index_idx++] = element_idx + 0;
  index_buf[index_idx++] = element_idx + 1;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 2;
  index_buf[index_idx++] = element_idx + 3;
  index_buf[index_idx++] = element_idx + 1;
}

int gl_get_text_width(const char *text, int len) {
  int res = 0;
  for (const char *p = text; *p && len--; p++) {
    if ((*p & 0xc0) == 0x80) { continue; }
    int chr = mu_min((unsigned char) *p, 127);
    res += atlas[ATLAS_FONT + chr].w;
  }
  return res;
}

int gl_get_text_height(void) {
  return 18;
}

void gl_draw_rect(mu_Rect rect, mu_Color color) {
  push_quad(rect, atlas[ATLAS_WHITE], color);
}

void gl_draw_icon(int id, mu_Rect rect, mu_Color color) {
  mu_Rect src = atlas[id];
  int x = rect.x + (rect.w - src.w) / 2;
  int y = rect.y + (rect.h - src.h) / 2;
  push_quad(mu_rect(x, y, src.w, src.h), src, color);
}

void gl_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
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

void gl_set_clip_rect(mu_Rect rect) {
  gl_flush();
  glScissor(rect.x, height - (rect.y + rect.h), rect.w, rect.h);
}

void gl_clear(mu_Color clr) {
  gl_flush();
  glClearColor(clr.r / 255., clr.g / 255., clr.b / 255., clr.a / 255.);
  glClear(GL_COLOR_BUFFER_BIT);
}

void gl_present(void) {
  gl_flush();
  SDL_GL_SwapWindow(window);
}
