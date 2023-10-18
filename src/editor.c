#include <assert.h>

#include "editor.h"
#include "render.h"
#include "freetype.h"
Glyph calculated_characters[CHAR_CAPACITY];
size_t calculated_character_size = 0;

Cursor cursors[MAX_CURSORS];
size_t num_cursors = 1;
float cursor_size[2];
float cursor_color[4] = {0.8, 0.9, 1.0, 1.0};

Editor main_editor;

void init_editor(Editor *editor) {
    char word[] = "";
    editor->text = malloc(sizeof(word));
    editor->size = sizeof(word);
    editor->capacity = sizeof(word);
    editor->cursor_x = 0;
    editor->cursor_y = 0;
    memcpy(editor->text, word, sizeof(word));

    // build line buffer
    editor->num_lines = 0;
    editor->line_start[0] = 0;
    for (size_t i = 0; i < editor->size; i++) {
        if (editor->text[i] == '\n' || editor->text[i] == '\0')
            editor->line_start[++editor->num_lines] = i + 1;
    }
}
size_t get_cursor_index(Editor *editor) {
    assert(editor->cursor_y < editor->num_lines);

    size_t line_start = editor->line_start[editor->cursor_y];
    size_t line_size = editor->line_start[editor->cursor_y + 1] - line_start;
    if (editor->cursor_x >= line_size) return line_start + line_size - 1;
    return line_start + editor->cursor_x;
}

void calculate(Editor *editor) {
    calculated_character_size = 0;
    unsigned int accumulated_width = 0, accumulated_height = 0;
    int line_height = (FONT_SIZE_PT * PPI) >> 6;
    int letter_spacing = 0;
    cursor_size[0] = CURSOR_WIDTH;
    cursor_size[1] = line_height;
    for (size_t i = 0; i < editor->size; ++i) {
        size_t index = (size_t) editor->text[i];

        calculated_characters[calculated_character_size] = (Glyph) {
            .uv_offset_x = char_params[index].offset,
            .uv_size = {char_params[index].uv_width, char_params[index].uv_height},
            .pos = {accumulated_width + char_params[index].bitmap_left, accumulated_height + line_height - char_params[index].bitmap_top},
            .size = {char_params[index].width, char_params[index].height},
            .bg_color = BG_COLOR,
            .fg_color = FG_COLOR
        };

        if (get_cursor_index(editor) == i) {
            cursors[0].pos[0] = accumulated_width;
            cursors[0].pos[1] = accumulated_height + 5;
        }

        calculated_character_size++;
        if (editor->text[i] == '\n') {
            accumulated_height += line_height;
            accumulated_width = 0;
            continue;
        }
        accumulated_width += char_params[index].advance_x + letter_spacing;
    }
}
void cursor_up(Editor *editor) {
    if (editor->cursor_y == 0)
        editor->cursor_x = 0;
    else
        editor->cursor_y -= 1;
}
void cursor_down(Editor *editor) {
    if (editor->cursor_y == editor->num_lines - 1) {
        size_t line_start = editor->line_start[editor->cursor_y];
        size_t line_size = editor->line_start[editor->cursor_y + 1] - line_start;
        editor->cursor_x = line_size - 1;
    } else
        editor->cursor_y += 1;
}
void cursor_left(Editor *editor) {
    size_t line_start = editor->line_start[editor->cursor_y];
    size_t line_size = editor->line_start[editor->cursor_y + 1] - line_start;
    if ((editor->cursor_x == 0 || line_size == 1) && editor->cursor_y > 0) { // at line start, wrap to previous line
    
        editor->cursor_y--;
        line_start = editor->line_start[editor->cursor_y];
        line_size = editor->line_start[editor->cursor_y + 1] - line_start;
        editor->cursor_x = line_size - 1;
    } else if (editor->cursor_x > 0 && line_size > 1) {
        if (editor->cursor_x >= line_size)
            editor->cursor_x = line_size - 2;
        else {
            editor->cursor_x--;
        }
    }
}
void cursor_right(Editor *editor) {
    size_t line_start = editor->line_start[editor->cursor_y];
    size_t line_size = editor->line_start[editor->cursor_y + 1] - line_start;
    if ((editor->cursor_x >= line_size - 1 || line_size == 1) && editor->cursor_y < editor->num_lines - 1) { // at line end, wrap to next line
        editor->cursor_y++;
        editor->cursor_x = 0;
    } else if (editor->cursor_x < line_size - 1 && line_size > 1) {
        editor->cursor_x++;
    }
}

void cursor_home(Editor *editor) {
    editor->cursor_x = 0;
}

void cursor_end(Editor *editor) {
    size_t line_start = editor->line_start[editor->cursor_y];
    size_t line_size = editor->line_start[editor->cursor_y + 1] - line_start;
    editor->cursor_x = line_size - 1;
}

void insert(Editor *e, char ch) {
    // Insert at cursor

    size_t pos = get_cursor_index(e);
    if (e->size == e->capacity) {
        size_t new_cap = e->capacity * 2;
        e->text = realloc(e->text, new_cap);
        e->capacity = new_cap;
    }
    memmove(e->text + pos + 1, e->text + pos, e->size - pos);
    e->text[pos] = ch;
    e->size += 1;
    // recalculate every line start after current line to +1;
    for (size_t i = e->cursor_y + 1; i <= e->num_lines; i++) {
        e->line_start[i]++;
    }

    if (ch == '\n') {
        assert(e->num_lines < MAX_LINES);
        memmove(e->line_start + e->cursor_y + 2, e->line_start + e->cursor_y + 1, (e->num_lines - e->cursor_y) * sizeof(size_t));
        e->line_start[e->cursor_y + 1] = pos + 1;
        e->num_lines++;
        e->cursor_x = 0;
        e->cursor_y++;
    } else {
        e->cursor_x = pos - e->line_start[e->cursor_y] + 1;
    }
}
void delete(Editor *e) {
    size_t pos = get_cursor_index(e);
    if (pos == e->size - 1) return;
    if (e->size <= e->capacity / 2) {
        size_t new_cap = e->capacity / 2;
        e->text = realloc(e->text, new_cap);
        e->capacity = new_cap;
    } 

    e->size -= 1;
    char deleted = e->text[pos];
    memmove(e->text + pos, e->text + pos + 1, e->size - pos);

    for (size_t i = e->cursor_y + 1; i <= e->num_lines; i++) {
        e->line_start[i]--;
    }

    if (deleted == '\n') {
        e->num_lines--;
        memmove(e->line_start + e->cursor_y + 1, e->line_start + e->cursor_y + 2, (e->num_lines - e->cursor_y) * sizeof(size_t));
    }
    e->cursor_x = pos - e->line_start[e->cursor_y];
}
void backspace(Editor *e) {
    size_t pos = get_cursor_index(e);
    if (pos == 0) return;
    pos -= 1;
    if (e->size <= e->capacity / 2) {
        size_t new_cap = e->capacity / 2;
        e->text = realloc(e->text, new_cap);
        e->capacity = new_cap;
    } 

    e->size -= 1;
    char deleted = e->text[pos];
    memmove(e->text + pos, e->text + pos + 1, e->size - pos);

    for (size_t i = e->cursor_y + 1; i <= e->num_lines; i++) {
        e->line_start[i]--;
    }

    if (deleted == '\n') {
        e->num_lines--;
        memmove(e->line_start + e->cursor_y + 1, e->line_start + e->cursor_y + 2, (e->num_lines - e->cursor_y) * sizeof(size_t));
        e->cursor_y--;
        e->cursor_x = pos - e->line_start[e->cursor_y];
    } else {
        e->cursor_x--;
    }
}

void insert_spaces(Editor *editor) {
    size_t effective_left = get_cursor_index(editor) - editor->line_start[editor->cursor_y];
    size_t new_left = ((effective_left / TAB_SIZE) + 1) * TAB_SIZE;
    for(size_t i = effective_left; i < new_left; i++) {
        insert(editor, ' ');
    }
}
gboolean handle_key_press(GtkGLArea *area,
                      guint keyval,
                      guint keycode,
                      GdkModifierType state,
                      GtkEventControllerKey *event_controller) {
    if (state & (GDK_CONTROL_MASK | GDK_ALT_MASK)) {
        return FALSE;
    }
    switch (keyval) {
        case GDK_KEY_Up:
            cursor_up(&main_editor);
            break;
        
        case GDK_KEY_Down:
            cursor_down(&main_editor);
            break;
        case GDK_KEY_Left:
            cursor_left(&main_editor);
            break;
        
        case GDK_KEY_Right:
            cursor_right(&main_editor);
            break;

        case GDK_KEY_Home:
            cursor_home(&main_editor);
            break;
        
        case GDK_KEY_End:
            cursor_end(&main_editor);
            break;
        
        case GDK_KEY_Return:
            insert(&main_editor, '\n');
            break;

        case GDK_KEY_BackSpace:
            backspace(&main_editor);
            break;

        case GDK_KEY_Delete:
            delete(&main_editor);
            break;

        case GDK_KEY_Tab:
            insert_spaces(&main_editor);
            break;

        default:
            if (keyval <= UCHAR_MAX)
                insert(&main_editor, (char)keyval);
            else
                return TRUE;
    }
    calculate(&main_editor);
    gtk_gl_area_queue_render(area);
    return TRUE;
}