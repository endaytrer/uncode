#ifndef _RENDER_H
#define _RENDER_H
#include <gtk/gtk.h>
#define CHAR_CAPACITY 65536
#define MAX_CURSORS 1
typedef struct {
    float pos[2];
    float size[2];
    float uv_offset_x;
    float uv_size[2];
    float fg_color[3];
    float bg_color[3];
} Glyph;

typedef struct {
    float pos[2];
} Cursor;

extern Glyph calculated_characters[CHAR_CAPACITY];
extern size_t calculated_character_size;

extern Cursor cursors[MAX_CURSORS];
extern size_t num_cursors;
extern float cursor_size[2];
extern float cursor_color[4];
// TODO: multiple cursor support

gboolean render(GtkGLArea *area, GdkGLContext *context);
void realize(GtkGLArea *area);
void unrealize(GtkGLArea *area);

#endif // _RENDER_H