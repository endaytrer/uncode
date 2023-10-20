#ifndef _RENDER_H
#define _RENDER_H
#include <gtk/gtk.h>
#include <sys/time.h>

#define CHAR_CAPACITY 65536
#define MAX_CURSORS 1
#define MAX_RECTS 1024

#define EPS 0.001f
#define DRAG 0.90f
#define SCROLL_SPEED 300

#include "renderables.h"

extern Glyph calculated_characters[CHAR_CAPACITY];
extern size_t calculated_character_size;


extern Rect rects[MAX_RECTS];
extern size_t num_rects;

extern float cursor_size[2];
extern float cursor_color[4];
// TODO: multiple cursor support


// viewport
extern int viewport_size[2];
extern float viewport_pos[2];
extern float viewport_velocity[2];
extern int viewport_scale;
extern time_t start_sec;
extern float last_edit;

gboolean render(GtkGLArea *area, GdkGLContext *context);
void realize(GtkGLArea *area);
void unrealize(GtkGLArea *area);

#endif // _RENDER_H