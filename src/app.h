#ifndef _APP_H
#define _APP_H

#include <gtk/gtk.h>
#include <adwaita.h>

#define UNCODE_APP_TYPE (uncode_app_get_type())


G_DECLARE_FINAL_TYPE(UncodeApp, uncode_app, UNCODE, APP, AdwApplication);

UncodeApp *uncode_app_new(void);
#endif // _APP_H