#ifndef _WINDOW_H
#define _WINDOW_H

#include <gtk/gtk.h>
#include "app.h"

#define UNCODE_APP_WINDOW_TYPE (uncode_app_window_get_type())

G_DECLARE_FINAL_TYPE(UncodeAppWindow, uncode_app_window, UNCODE, APP_WINDOW, GtkApplicationWindow);

UncodeAppWindow *uncode_app_window_new(UncodeApp *app);
void uncode_app_window_open(UncodeAppWindow *win, GFile *file);

#endif // _WINDOW_H