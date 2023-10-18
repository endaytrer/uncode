#include <gtk/gtk.h>
#include "app.h"
#include "window.h"
#include "render.h"
#include "editor.h"

struct _UncodeAppWindow {
    GtkApplicationWindow parent;
};

G_DEFINE_TYPE(UncodeAppWindow, uncode_app_window, GTK_TYPE_APPLICATION_WINDOW);

static void uncode_app_window_init(UncodeAppWindow *app) {

}

static void uncode_app_window_class_init(UncodeAppWindowClass *class) {

}

UncodeAppWindow *uncode_app_window_new(UncodeApp *app){
    UncodeAppWindow *win = g_object_new(UNCODE_APP_WINDOW_TYPE, "application", app, NULL);

    gtk_window_set_title (GTK_WINDOW (win), "GL Area");
    gtk_window_set_default_size (GTK_WINDOW (win), 800, 600);


    GtkWidget *gl_area = gtk_gl_area_new();
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

    GtkEventController *controller = gtk_event_controller_key_new();

    g_signal_connect_object(controller, "key-pressed", G_CALLBACK(handle_key_press), gl_area, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(GTK_WIDGET(win), controller);

    gtk_window_set_child(GTK_WINDOW(win), gl_area);
    return win;
}

void uncode_app_window_open(UncodeAppWindow *win, GFile *file) {
    char *path = g_file_get_path(file);
    init_editor(&main_editor, path);
    
}