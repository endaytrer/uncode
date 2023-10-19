#include <gtk/gtk.h>
#include <assert.h>

#include "app.h"
#include "window.h"
#include "render.h"
#include "editor.h"


struct _UncodeApp {
    AdwApplication parent;
};

G_DEFINE_TYPE(UncodeApp, uncode_app, ADW_TYPE_APPLICATION);

static void uncode_app_init(UncodeApp *app) {
    (void)app;
}

static void uncode_app_activate(GApplication* app) {
    UncodeAppWindow *win = uncode_app_window_new(UNCODE_APP(app));
    init_editor(&main_editor, NULL);
    gtk_window_present(GTK_WINDOW(win));
}

static void uncode_app_open(GApplication *app, GFile **files, int n_files, const char *hint) {
    (void)hint;
    
    assert(n_files == 1);
    GList *windows;
    UncodeAppWindow *win;
    int i;
    windows = gtk_application_get_windows(GTK_APPLICATION(app));
    if (windows)
        win = UNCODE_APP_WINDOW(windows->data);
    else
        win = uncode_app_window_new(UNCODE_APP(app));

    for (i = 0; i < n_files; i++) {
        uncode_app_window_open(win, files[i]);
    }
    gtk_window_present(GTK_WINDOW(win));
}

static void uncode_app_class_init(UncodeAppClass *class) {
    G_APPLICATION_CLASS(class)->activate = uncode_app_activate;
    G_APPLICATION_CLASS(class)->open = uncode_app_open;
}

UncodeApp *uncode_app_new(void) {
    return g_object_new(UNCODE_APP_TYPE,
        "application-id", "org.endaytrer.uncode",
        "flags", G_APPLICATION_HANDLES_OPEN,
        NULL);
}