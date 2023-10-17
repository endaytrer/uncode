#ifndef _RENDER_H
#define _RENDER_H
#include <gtk/gtk.h>
#define CHAR_CAPACITY 65536
typedef struct {
    float pos[2];
    float size[2];
    float ch;
    float color[3];
} Vertex;

extern Vertex calculated_characters[CHAR_CAPACITY];
extern size_t calculated_character_size;
gboolean render(GtkGLArea *area, GdkGLContext *context);
void realize(GtkGLArea *area);
void unrealize(GtkGLArea *area);

#endif // _RENDER_H