#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <assert.h>

#include "app.h"



int main(int argc, char **argv){
    return g_application_run(G_APPLICATION(uncode_app_new()), argc, argv);
}

