#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "render.h"
#include "editor.h"



static void activate (GtkApplication* app) {
    GtkWidget *window;

    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "GL Area");
    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);


    GtkWidget *gl_area = gtk_gl_area_new();

    g_signal_connect(gl_area, "realize", G_CALLBACK(realize), NULL);
    g_signal_connect(gl_area, "unrealize", G_CALLBACK(unrealize), NULL);
    g_signal_connect(gl_area, "render", G_CALLBACK(render), NULL);

    GtkEventController *controller = gtk_event_controller_key_new();

    g_signal_connect_object(controller, "key-pressed", G_CALLBACK(handle_key_press), gl_area, G_CONNECT_SWAPPED);
    gtk_widget_add_controller(window, controller);

    gtk_window_set_child(GTK_WINDOW (window), gl_area);
    gtk_window_present(GTK_WINDOW(window));
}

int main (int argc, char **argv){
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.endaytrer.example", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref(app);

    return status;
}

