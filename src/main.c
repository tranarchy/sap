#include <pwd.h>
#include <math.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include <microui.h>
#include <renderer.h>

#define VISUALIZER_BARS 32
#define MIN_DBFS (-98.09f)

static int _argc = 0;
static char **_argv;

static char music_dir[1024];

static char **drag_and_drop_dirs;
static int drag_and_drop_count = 0;

static Mix_Music *music = NULL;
static float music_pos = 0;
static unsigned char volume = MIX_MAX_VOLUME;

static float dBFS_data[VISUALIZER_BARS] = { MIN_DBFS };

static char queue[1024][1024];
static int queue_count = 0;
static int queue_selected = 0;

static int shuffle = 0;

static mu_Color color;

static void music_finished(void) {
    if (shuffle) {
      queue_selected = rand() % queue_count;
    } else {
      if (queue_selected + 1 < queue_count) {
        queue_selected++;
      } else {
        queue_selected = 0;
      }
    }

    music = Mix_LoadMUS(queue[queue_selected]);

    Mix_PlayMusic(music, 1);
}

static void music_hook(void *udata, Uint8 *stream, int len) {
  if (queue_count == 0) {
      Mix_FreeMusic(music);
      music = NULL;
  }

  int16_t *samples = (int16_t*)stream;

  int total_sampels = len / sizeof(int16_t);

  double sum_of_squares = 0.0;

  for (int i = 0; i < total_sampels; i++) {
    float normalized_sample = (float)samples[i] / 32767.0f;

    sum_of_squares = (double)normalized_sample * normalized_sample;
  }

  double avg_square = sum_of_squares / total_sampels;

  float rms_amplitude = (float)sqrt(avg_square);

  float rms_dBFS = rms_amplitude > 0.0f ? 20.0f * log10f(rms_amplitude) : MIN_DBFS;
  
  for (int i = 0; i < VISUALIZER_BARS - 1; i++) {
     dBFS_data[i] = dBFS_data[i+1];
  }

  dBFS_data[VISUALIZER_BARS - 1] = rms_dBFS;
}

static int text_width(mu_Font font, const char *text, int len) {
  if (len == -1) { len = strlen(text); }
  return sdlr_get_text_width(text, len);
}

static int text_height(mu_Font font) {
  return sdlr_get_text_height();
}

static char *strip_file(char *entry) {
    int slash_pos = 0;
    int extension_pos = -1;

    for (int i = 0; i < strlen(entry); i++) {
        if (entry[i] == '/') {
          slash_pos = i + 1;
        }

        if (entry[i] == '.') {
          extension_pos = i;
        }
    }

    char *stripped_entry = strdup(entry + slash_pos);

    if (extension_pos != -1) {
      stripped_entry[extension_pos - slash_pos] = '\0';
    }
  
    return stripped_entry;
}

static void truncate_text(mu_Context *ctx, char *text) {
    mu_Container *container =  mu_get_current_container(ctx);
    int width = container->rect.w;
  
    for (int i = strlen(text); i > 0; i--) {
      int t_width = text_width(NULL, text, -1) + 15;
      
      if (t_width > width) {
        text[i] = '\0';
      } else {
        break;
      }
    }
}

static void add_to_queue(const char *music_path) {
   strlcpy(queue[queue_count], music_path, 1024);
  
   if (queue_count == 1023) {
       queue_count = 0;
    } else {
       queue_count++;
    }
}

static void remove_from_queue(char *music_path) {
    bool move_to_left = false;

    int removed_idx = -1;

    for (int i = 0; i < queue_count; i++) {
        if (strcmp(queue[i], music_path) == 0) {
          move_to_left = true;
          removed_idx = i;
          continue;
        }
        
        if (move_to_left) {
          strlcpy(queue[i - 1], queue[i], 1024);
        }
    }

    queue_count--;

    if (removed_idx == queue_selected) {
      if (queue_selected == queue_count) {
        music_finished();
      } else {
        music = Mix_LoadMUS(queue[queue_selected]);
        Mix_PlayMusic(music, 1);
      }
    } else if (removed_idx < queue_selected) {
      queue_selected--;
    }
}

static void get_tracks(mu_Context *ctx, char *dir_path) {    
    struct dirent *entry;

    char abs_entry_name[1024];

    DIR *dir = opendir(dir_path);

    bool add_dir = false;

    char *stripped_dir = strip_file(dir_path);
    truncate_text(ctx, stripped_dir);

    if (ctx->hover == mu_get_id(ctx, stripped_dir, strlen(stripped_dir))) {
      if (ctx->mouse_pressed == MU_MOUSE_RIGHT) {
         add_dir = true;
      }
    }

    if (mu_header(ctx, stripped_dir)) {      
      while ((entry = readdir(dir)) != NULL) {
          if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
              continue;

          snprintf(abs_entry_name, 1024, "%s/%s", dir_path, entry->d_name);        

          if (entry->d_type == DT_DIR) {
              get_tracks(ctx, abs_entry_name);
          } else if (entry->d_type == DT_REG) {
             Mix_Music *temp_music = Mix_LoadMUS(abs_entry_name);
             truncate_text(ctx, entry->d_name);
             if (temp_music != NULL) {
                bool duplicate = false;
             
                mu_layout_row(ctx, 2, (int[]) { text_width(NULL, entry->d_name, -1) + 5, -1 }, 0);
                if (mu_button(ctx, entry->d_name) || add_dir) {
                
                  for (int i = 0; i < queue_count; i++) {
                    if (strcmp(queue[i], abs_entry_name) == 0) {
                      duplicate = true;
                      break;
                    }
                  }

                  if (!duplicate) {
                    add_to_queue(abs_entry_name);
                  }
                }
             }
             Mix_FreeMusic(temp_music);
          }
        }
    }

    free(stripped_dir);

    closedir(dir);
}

static void player_window(mu_Context *ctx) {
  if (mu_begin_window_ex(ctx, "Player", mu_rect(44, 325, 348, 115), MU_OPT_NOCLOSE)) {
      mu_layout_row(ctx, 1, (int[]) { -1 }, 0);

      music_pos = Mix_GetMusicPosition(music);

      if (music != NULL && queue_count > 0) {
        char currently_plaing[2048];
        char *stripped_file = strip_file(queue[queue_selected]);
        
        snprintf(currently_plaing, 2048, "%s (%d / %d)", stripped_file, queue_selected + 1, queue_count);
        truncate_text(ctx, currently_plaing);

        mu_draw_control_text(ctx, currently_plaing, mu_layout_next(ctx), MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
        
        free(stripped_file);
      }
      
      
      if (mu_slider(ctx, &music_pos, 0, Mix_MusicDuration(music))) {
        Mix_SetMusicPosition(music_pos);
      }

      if (ctx->key_pressed == MU_KEY_RIGHT || ctx->key_pressed == MU_KEY_LEFT) {
         int music_duration = Mix_MusicDuration(music);
         float jump = ctx->key_down == (MU_KEY_SHIFT | ctx->key_pressed) ? 10 : 5; 
         
         if (ctx->key_pressed == MU_KEY_LEFT) {
            if (music_pos - jump < 0) jump = music_pos;

            jump = -jump;
         } else {
            if (music_duration < music_pos + jump) {
              music_finished();
              music_pos = 0;
              jump = 0;
            }
         }
         
         Mix_SetMusicPosition(music_pos + jump);
      }
      
      mu_layout_row(ctx, 3, (int[]) { 86, -110, -1 }, 0);
      if (mu_button(ctx, "<") || 
      (ctx->key_down == (MU_KEY_CTRL | MU_KEY_LEFT) && ctx->key_pressed == MU_KEY_LEFT)) { 
       if (queue_selected > 0) {
          queue_selected--;
        } else {
          queue_selected = 0;
        }
        
        music = Mix_LoadMUS(queue[queue_selected]);
        Mix_PlayMusic(music, 1);
      }
      
      if (mu_button(ctx, "||") || ctx->key_pressed == MU_KEY_SPACE) {
        if (queue_count > 0) {
        
          if (music == NULL) music = Mix_LoadMUS(queue[queue_selected]);

          Mix_PausedMusic() ? Mix_ResumeMusic() : Mix_PauseMusic();
          
          if (Mix_PlayingMusic() == 0) {
          Mix_PlayMusic(music, 1);
          }
        }
      }
      
      if (mu_button(ctx, ">") || 
      (ctx->key_down == (MU_KEY_CTRL | MU_KEY_RIGHT) && ctx->key_pressed == MU_KEY_RIGHT)) { 
        music_finished();
      }

    mu_end_window(ctx);
  }
}

static void visualizer_window(mu_Context *ctx) {
   if (mu_begin_window(ctx, "Visualizer", mu_rect(60, 60, VISUALIZER_BARS * 10, 200))) {
      mu_Container *win = mu_get_current_container(ctx);
 
      int x = win->rect.x;
      int y = win->rect.y;

      int width = win->rect.w / VISUALIZER_BARS;
      int height = win->rect.h;
      
      for (int i = 0; i < VISUALIZER_BARS; i++) {
        float dBFS = dBFS_data[i];

        float dBFS_percent = (dBFS + -MIN_DBFS) / -MIN_DBFS;

        int bar_y = height + y - (height * dBFS_percent);

        mu_Rect rect = mu_rect(x, bar_y, width, height * dBFS_percent);
        x += width; 

       mu_draw_box(ctx, rect, color);
      }

      mu_end_window(ctx);
   }
}

static void files_window(mu_Context *ctx) {
  struct stat source_stat;
   
  char path[1024];

  if (mu_begin_window_ex(ctx, "Files", mu_rect(426, 40, 300, 200), MU_OPT_NOCLOSE)) {

    mu_layout_row(ctx, 1, (int[]) { -1 }, -25);

    get_tracks(ctx, music_dir);
    for (int i = 1; i < _argc; i++) {
      
      int ret = stat(_argv[i], &source_stat);

      if (ret == -1) {
        continue;
      }

      if (!S_ISDIR(source_stat.st_mode)) {
        continue;
      }
      
      realpath(_argv[i], path);
      get_tracks(ctx, path);
    }

    for (int i = 0; i < drag_and_drop_count; i++) {
      get_tracks(ctx, drag_and_drop_dirs[i]);
    }
    
    mu_end_window(ctx);
  }
}

static void queue_window(mu_Context *ctx) {
  if (mu_begin_window_ex(ctx, "Queue", mu_rect(426, 266, 300, 200), MU_OPT_NOCLOSE)) {

    mu_layout_row(ctx, 1, (int[]) { -1 }, -25);
    mu_begin_panel(ctx, "Files");
  
    mu_layout_row(ctx, 1, (int[]) { -1 }, 0);
    
    for (int i = 0; i < queue_count; i++) {
      
      char *stripped_file = strip_file(queue[i]);
      truncate_text(ctx, stripped_file);

      if (ctx->hover == mu_get_id(ctx, stripped_file, strlen(stripped_file))) {
        if (ctx->mouse_pressed == MU_MOUSE_MIDDLE) {
          remove_from_queue(queue[i]);
        }
      }
      
      if (mu_button(ctx, stripped_file)) { 
          queue_selected = i;
          music = Mix_LoadMUS(queue[queue_selected]);
          Mix_PlayMusic(music, 1);  
      };
      
      free(stripped_file);
    }
   
    mu_end_panel(ctx);
    
    mu_layout_row(ctx, 3, (int[]) { 86, 30, 50 }, -1);
    if (mu_button(ctx, "Clear")) {
      queue_selected = 0;
      queue_count = 0;
    }

    mu_checkbox(ctx, "Shuffle", &shuffle);

    mu_end_window(ctx);
  }
}

static int uint8_slider(mu_Context *ctx, unsigned char *value, int low, int high) {
  static float tmp;
  mu_push_id(ctx, &value, sizeof(value));
  tmp = *value;
  int res = mu_slider_ex(ctx, &tmp, low, high, 0, "%.0f", MU_OPT_ALIGNCENTER);
  *value = tmp;
  mu_pop_id(ctx);
  return res;
}

static void settings_window(mu_Context *ctx) {
  if (mu_begin_window_ex(ctx, "Settings", mu_rect(273, 490, 241, 168), MU_OPT_NOCLOSE)) {
    mu_layout_row(ctx, 2, (int[]) { 70, 150 }, 0);
    mu_label(ctx, "Volume");

    if (uint8_slider(ctx, &volume, 0, MIX_MAX_VOLUME)) {
      Mix_VolumeMusic(volume);
    }

    if (ctx->key_pressed == MU_KEY_UP || ctx->key_pressed == MU_KEY_DOWN) {
      volume += ctx->key_pressed == MU_KEY_UP ? 5 : -5;
      Mix_VolumeMusic(volume);
    }

    
    mu_label(ctx, "Vis R"); uint8_slider(ctx, &color.r, 0, 255);
    mu_label(ctx, "Vis G"); uint8_slider(ctx, &color.g, 0, 255);
    mu_label(ctx, "Vis B"); uint8_slider(ctx, &color.b, 0, 255);
    mu_label(ctx, "Vis A"); uint8_slider(ctx, &color.a, 0, 255);

    mu_end_window(ctx);
  }
}


static void process_frame(mu_Context *ctx) {
  mu_begin(ctx);
  visualizer_window(ctx);
  player_window(ctx);
  files_window(ctx);
  queue_window(ctx);
  settings_window(ctx);
  mu_end(ctx);
}

static const char button_map[256] = {
  [ SDL_BUTTON_LEFT   & 0xff ] =  MU_MOUSE_LEFT,
  [ SDL_BUTTON_RIGHT  & 0xff ] =  MU_MOUSE_RIGHT,
  [ SDL_BUTTON_MIDDLE & 0xff ] =  MU_MOUSE_MIDDLE,
};

static const short key_map[256] = {
  [ SDLK_LSHIFT       & 0xff ] = MU_KEY_SHIFT,
  [ SDLK_RSHIFT       & 0xff ] = MU_KEY_SHIFT,
  [ SDLK_LCTRL        & 0xff ] = MU_KEY_CTRL,
  [ SDLK_RCTRL        & 0xff ] = MU_KEY_CTRL,
  [ SDLK_BACKSPACE    & 0xff ] = MU_KEY_BACKSPACE,
  [ SDLK_SPACE        & 0xff ] = MU_KEY_SPACE,
  [ SDLK_RIGHT        & 0xff ] = MU_KEY_RIGHT,
  [ SDLK_LEFT         & 0xff ] = MU_KEY_LEFT,
  [ SDLK_UP           & 0xff ] = MU_KEY_UP,
  [ SDLK_DOWN         & 0xff ] = MU_KEY_DOWN,
};

int main(int argc, char **argv) {
  struct stat source_stat;

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
  Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024);
  Mix_HookMusicFinished(music_finished);
  Mix_SetPostMix(music_hook, NULL);
  sdlr_init();

  mu_Context *ctx = malloc(sizeof(mu_Context));
  mu_init(ctx);
  ctx->text_width = text_width;
  ctx->text_height = text_height;

  color = mu_color(95, 68, 196, 255);

  for (int i = 1; i < argc; i++) {
  
    int ret = stat(argv[i], &source_stat);

    if (ret == -1) {
      continue;
    }
    
    if (S_ISREG(source_stat.st_mode)) {
      add_to_queue(argv[i]);
    }
  }

  _argc = argc;
  _argv = argv;

  struct passwd *pw = getpwuid(getuid());
  snprintf(music_dir, 1024, "%s/Music", pw->pw_dir);

  int ret = stat(music_dir, &source_stat);

  if (ret == -1) {
    mkdir(music_dir, 16877);
  }

  drag_and_drop_dirs = malloc(sizeof(char*));
 
  for (;;) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          free(ctx);

          for (int i = 0; i < drag_and_drop_count; i++) {
            free(drag_and_drop_dirs[i]);
          }
          
          free(drag_and_drop_dirs);

          Mix_FreeMusic(music);
          exit(EXIT_SUCCESS); 
          break;
        case SDL_MOUSEMOTION: mu_input_mousemove(ctx, e.motion.x, e.motion.y); break;
        case SDL_MOUSEWHEEL: mu_input_scroll(ctx, 0, e.wheel.y * -30); break;
        //case SDL_TEXTINPUT: mu_input_text(ctx, e.text.text); break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
          int b = button_map[e.button.button & 0xff];
          if (b && e.type == SDL_MOUSEBUTTONDOWN) { mu_input_mousedown(ctx, e.button.x, e.button.y, b); }
          if (b && e.type ==   SDL_MOUSEBUTTONUP) { mu_input_mouseup(ctx, e.button.x, e.button.y, b);   }
          break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP: {
          int c = key_map[e.key.keysym.sym & 0xff];
          if (c && e.type == SDL_KEYDOWN) { mu_input_keydown(ctx, c); }
          if (c && e.type ==   SDL_KEYUP) { mu_input_keyup(ctx, c);   }
          break;
        }

        case SDL_DROPFILE: {
          int ret = stat(e.drop.file, &source_stat);

          if (ret == -1) {
            continue;
          }
          
          if (S_ISREG(source_stat.st_mode)) {
            add_to_queue(e.drop.file);
          } else if (S_ISDIR(source_stat.st_mode)) {
            drag_and_drop_count++;
            
            drag_and_drop_dirs = reallocarray(drag_and_drop_dirs, drag_and_drop_count, sizeof(char *));
            drag_and_drop_dirs[drag_and_drop_count - 1] = malloc((strlen(e.drop.file) + 1) * sizeof(char));
                        
            strlcpy(drag_and_drop_dirs[drag_and_drop_count - 1], e.drop.file, strlen(e.drop.file) + 1);
          }
          
          SDL_free(e.drop.file);
        }
      }
    }

    process_frame(ctx);

    sdlr_clear(mu_color(10, 10, 23, 255));
    mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
      switch (cmd->type) {
        case MU_COMMAND_TEXT: sdlr_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: sdlr_draw_rect(cmd->rect.rect, cmd->rect.color); break;
        case MU_COMMAND_ICON: sdlr_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
        case MU_COMMAND_CLIP: sdlr_set_clip_rect(cmd->clip.rect); break;
      }
    }
    sdlr_present();
  }

  return 0;
}

