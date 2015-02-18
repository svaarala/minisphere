#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include "duktape.h"

static const char* SPHERE_API_VER = "v1.5";
static const char* ENGINE_VER = "v0.0";

struct key_queue
{
	int num_keys;
	int keys[255];
};

typedef struct key_queue key_queue_t;

extern ALLEGRO_DISPLAY*     g_display;
extern ALLEGRO_EVENT_QUEUE* g_events;
extern duk_context*         g_duktape;
extern int                  g_fps;
extern key_queue_t          g_key_queue;
extern int                  g_render_scale;
extern ALLEGRO_FONT*        g_sys_font;

extern void            al_draw_tiled_bitmap (ALLEGRO_BITMAP* bitmap, float x, float y, float width, float height);
extern ALLEGRO_BITMAP* al_fread_bitmap      (ALLEGRO_FILE* file, int width, int height);

extern bool  do_events(void);
extern bool  end_frame          (int framerate);
extern char* get_asset_path     (const char* path, const char* base_dir, bool allow_mkdir);
extern char* get_sys_asset_path (const char* path, const char* base_dir);