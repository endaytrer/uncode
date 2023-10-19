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
    (void)app;
}

static void uncode_app_window_class_init(UncodeAppWindowClass *class) {
    (void)class;
}

UncodeAppWindow *uncode_app_window_new(UncodeApp *app){
    UncodeAppWindow *win = g_object_new(UNCODE_APP_WINDOW_TYPE, "application", app, NULL);

    gtk_window_set_title (GTK_WINDOW (win), "GL Area");
    gtk_window_set_default_size (GTK_WINDOW (win), 800, 600);


    GtkWidget *gl_area = gtk_gl_area_new();
    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

    GtkEventController *key_controller = gtk_event_controller_key_new();
    g_signal_connect_object(key_controller, "key-pressed", G_CALLBACK(handle_key_press), gl_area, G_CONNECT_SWAPPED);


    GtkEventController *scroll_controller = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
    g_signal_connect(scroll_controller, "scroll", G_CALLBACK(handle_scroll), NULL);

    GtkGesture *gesture_controller = gtk_gesture_click_new();
    g_signal_connect(gesture_controller, "begin", G_CALLBACK(handle_mouse_down), NULL);
    g_signal_connect(gesture_controller, "end", G_CALLBACK(handle_mouse_up), NULL);

    GdkCursor *cursor = gdk_cursor_new_from_name("text", NULL);
    gtk_widget_set_cursor(GTK_WIDGET(gl_area), cursor);
    gtk_widget_add_controller(GTK_WIDGET(win), key_controller);
    gtk_widget_add_controller(GTK_WIDGET(gl_area), scroll_controller);
    gtk_widget_add_controller(GTK_WIDGET(gl_area), GTK_EVENT_CONTROLLER(gesture_controller));
    gtk_window_set_child(GTK_WINDOW(win), gl_area);
    return win;
}

void uncode_app_window_open(UncodeAppWindow *win, GFile *file) {
    (void)win;
    char *path = g_file_get_path(file);
    init_editor(&main_editor, path);
    
}