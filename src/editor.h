#ifndef _EDITOR_H
#define _EDITOR_H
#include <stddef.h>
#include <gtk/gtk.h>
#define MAX_LINES 4095
#define CURSOR_WIDTH 2
#define BG_COLOR {0.15, 0.15, 0.15}
#define FG_COLOR {1.0, 1.0, 1.0}
#define TAB_SIZE 4
typedef struct {
    char *text;
    size_t size; // including trailing '\0'
    size_t capacity;

    size_t cursor_x;
    size_t cursor_y;

    struct {
        size_t start;
        float render_length;
    } lines[MAX_LINES + 1];

    size_t num_lines;

} Editor;

extern Editor main_editor;

void init_editor(Editor *editor, char *file);
void calculate_render_length(Editor *editor);
void calculate(Editor *editor);
gboolean handle_key_press(
    GtkGLArea *area,
    guint keyval,
    guint keycode,
    GdkModifierType state,
    GtkEventControllerKey *event_controller
);
gboolean handle_scroll(
    GtkEventControllerScroll* self,
    gdouble dx,
    gdouble dy,
    gpointer user_data
);
#endif // _EDITOR_H