#ifndef _RENDER_H
#define _RENDER_H
#include <gtk/gtk.h>

gboolean render(GtkGLArea *area, GdkGLContext *context);
void realize(GtkGLArea *area);
void unrealize(GtkGLArea *area);

#endif // _RENDER_H