#ifndef _EDITOR_H
#define _EDITOR_H
#include <stddef.h>
#include <gtk/gtk.h>

#define CURSOR_WIDTH (((FONT_SIZE_PT * PPI) >> 6) / 6)
#define CURSOR_OFFSET_Y (((FONT_SIZE_PT * PPI) >> 6) / 4)

#define MAX_LINES 4095
#define BG_COLOR {0.15, 0.15, 0.15}
#define FG_COLOR {1.0, 1.0, 1.0}
#define CURSOR_COLOR {0.75, 0.95, 1.0}
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
extern bool layout_updated;

void init_editor(Editor *editor, char *file);
void calculate_render_length(Editor *editor);
void adjust_screen_text_area(Editor *editor);
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
void handle_mouse_down (
    GtkGesture* self,
    GdkEventSequence* sequence,
    gpointer user_data
);
void handle_mouse_up (
    GtkGesture* self,
    GdkEventSequence* sequence,
    gpointer user_data
);
#endif // _EDITOR_H