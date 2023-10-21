#ifndef _EDITOR_H
#define _EDITOR_H
#include <stddef.h>
#include <gtk/gtk.h>

#define CURSOR_WIDTH (((FONT_SIZE_PT * PPI) >> 6) / 6)
#define CURSOR_OFFSET_Y (((FONT_SIZE_PT * PPI) >> 6) / 4)

// offset for line number
#define MARGIN_LEFT 55
// line number from content
#define NUMBER_PADDING 10
#define NUMBER_MARGIN_RIGHT 10

#define MAX_LINES 4095
#define BG_COLOR {0.15, 0.15, 0.15, 1.0}
#define FG_COLOR {1.0, 1.0, 1.0}
#define NUMBER_COLOR {0.5, 0.5, 0.5}
#define NUMBER_BG_COLOR {0.15, 0.15, 0.15, 1.0}
#define CURRENT_NUMBER_COLOR {0.8, 0.8, 0.8}
#define CURRENT_LINE_COLOR {0.75, 0.95, 1.0, 0.03}
#define SELECTION_COLOR {0.75, 0.95, 1.0, 0.25}
#define CURSOR_COLOR {0.75, 0.95, 1.0, 1.0}
#define TAB_SIZE 4

typedef struct {
    size_t x;
    size_t y;
} Cursor;

typedef struct {
    char *text;
    size_t size; // including trailing '\0'
    size_t capacity;
    enum {
        Normal,
        Selection
    } mode;

    Cursor cursor;
    Cursor selection_start; // guaranteed to have normalized x (a.k.a. inside [0, line_length))

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
void calculate(Editor *editor, float time_since_key_pressed_sec);
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
void handle_mouse_move(
    GtkEventControllerMotion* self,
    gdouble x,
    gdouble y,
    gpointer user_data
);



#endif // _EDITOR_H