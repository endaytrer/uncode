#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "editor.h"
#include "render.h"
#include "freetype.h"
Glyph calculated_characters[CHAR_CAPACITY];
size_t calculated_character_size = 0;

Cursor cursors[MAX_CURSORS];
size_t num_cursors = 1;

int line_height;

float cursor_size[2];
float cursor_color[3] = CURSOR_COLOR;

int viewport_size[2];
float viewport_pos[2] = {0, 0};
float viewport_velocity[2] = {0, 0};

int viewport_scale = 2;
int prev_viewport_size_hash = 0;

int hash_size(void) {
    return viewport_size[0] * 28367 + viewport_size[1];
}
Editor main_editor = {
    .text = 0,
    .size = 0, // including trailing '\0'
    .capacity = 0,

    .cursor_x = 0,
    .cursor_y = 0,

    .lines = {},
    .num_lines = 0,

};
extern char *filename;
void init_editor(Editor *editor, char *file) {
    if (!file) {
        char word[] = "";
        if (editor->text) free(editor->text);
        editor->text = malloc(sizeof(word));
        editor->size = sizeof(word);
        editor->capacity = sizeof(word);
        memcpy(editor->text, word, sizeof(word));
    } else {
        int fd;
        if ((fd = open(file, O_RDONLY)) < 0) {
            fprintf(stderr, "cannot open file %s\n", file);
            exit(-1);
        }
        struct stat s;
        if (fstat(fd, &s) < 0) {
            fprintf(stderr, "fstat\n");
            exit(-1);
        }
        void *map;
        if ((map = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED) {
            fprintf(stderr, "mmap\n");
            exit(-1);
        }
        if (editor->text) free(editor->text);
        editor->text = malloc(s.st_size + 1);
        editor->size = s.st_size + 1;
        editor->capacity = s.st_size + 1;
        memcpy(editor->text, map, s.st_size);
        editor->text[s.st_size] = '\0';
        munmap(map, s.st_size);
        close(fd);
    }
    editor->cursor_x = 0;
    editor->cursor_y = 0;

    // build line buffer
    editor->num_lines = 0;
    editor->lines[0].start = 0;
    for (size_t i = 0; i < editor->size; i++) {
        if (editor->text[i] == '\n' || i == editor->size - 1) {
            editor->lines[++editor->num_lines].start = i + 1;
        }
    }
}
void calculate_render_length(Editor *editor) {
    for (size_t line = 0; line < editor->num_lines; line++) {
        editor->lines[line].render_length = 0;
        for (size_t i = editor->lines[line].start; i != editor->lines[line + 1].start; i++) {
            editor->lines[line].render_length += char_params[(size_t)editor->text[i]].advance_x;
        }
    }
}
size_t get_cursor_index(Editor *editor) {
    assert(editor->cursor_y < editor->num_lines);

    size_t line_start = editor->lines[editor->cursor_y].start;
    size_t line_size = editor->lines[editor->cursor_y + 1].start - line_start;
    if (editor->cursor_x >= line_size) return line_start + line_size - 1;
    return line_start + editor->cursor_x;
}

void get_cursor_pos(Editor *editor, float *x, float *y) {
    *y = editor->cursor_y * line_height + CURSOR_OFFSET_Y * viewport_scale;
    size_t line_start = editor->lines[editor->cursor_y].start;
    size_t effective_left = get_cursor_index(editor) - line_start;
    *x = 0;
    for (size_t i = 0; i < effective_left; i++) {
        char ch = editor->text[line_start + i];
        *x += char_params[(size_t)ch].advance_x;
    }
}

void get_text_area_size(Editor *editor, float *w, float *h) {
    *h = editor->num_lines * line_height + CURSOR_OFFSET_Y * viewport_scale;
    *w = 0;
    for (size_t i = 0; i < editor->num_lines; i++) {
        if (editor->lines[i].render_length > *w) {
            *w = editor->lines[i].render_length;
        }
    }
}

bool layout_updated = true;
void calculate(Editor *editor) {
    int hash = hash_size();
    if (!layout_updated && hash == prev_viewport_size_hash) return;

    line_height = viewport_scale * ((FONT_SIZE_PT * PPI) >> 6);

    layout_updated = false;
    prev_viewport_size_hash = hash;
    calculated_character_size = 0;
    num_cursors = 0;
    cursor_size[0] = CURSOR_WIDTH * viewport_scale;
    cursor_size[1] = line_height;
    if (editor->size == 0) return;

    get_cursor_pos(editor, cursors[0].pos, cursors[0].pos + 1);

    if (cursors[0].pos[0] <= (viewport_pos[0] + viewport_size[0]) * viewport_scale &&
        cursors[0].pos[0] + cursor_size[0] >= viewport_pos[0] * viewport_scale &&
        cursors[0].pos[1] <= (viewport_pos[1] + viewport_size[1]) * viewport_scale &&
        cursors[0].pos[1] + cursor_size[1] >= viewport_pos[1] * viewport_scale) {
        num_cursors = 1;
    }
    int start = viewport_pos[1] * viewport_scale / line_height - 2;
    size_t line = start < 0 ? 0 : start;
    size_t end = (viewport_pos[1] + viewport_size[1]) * viewport_scale / line_height + 1;
    if (editor->num_lines - 1 < end)
        end = editor->num_lines - 1;
    for (; line <= end; line++) {
        size_t accumulated_width = 0;
        size_t top = line * line_height;
        for (size_t i = editor->lines[line].start; i < editor->lines[line + 1].start; i++) {

            size_t index = (size_t) editor->text[i];

            calculated_characters[calculated_character_size] = (Glyph) {
                .uv_offset_x = char_params[index].offset,
                .uv_size = {char_params[index].uv_width, char_params[index].uv_height},
                .pos = {accumulated_width + char_params[index].bitmap_left, top + line_height - char_params[index].bitmap_top},
                .size = {char_params[index].width, char_params[index].height},
                .fg_color = FG_COLOR
            };
            accumulated_width += char_params[index].advance_x;

            if (calculated_characters[calculated_character_size].pos[0] > (viewport_pos[0] + viewport_size[0]) * viewport_scale) {
                break;
            }
            if (calculated_characters[calculated_character_size].pos[0] + calculated_characters[calculated_character_size].size[0] < viewport_pos[0] * viewport_scale ||
                calculated_characters[calculated_character_size].pos[1] > (viewport_pos[1] + viewport_size[1]) * viewport_scale ||
                calculated_characters[calculated_character_size].pos[1] + calculated_characters[calculated_character_size].size[1] < viewport_pos[1] * viewport_scale
                ) {
                continue;
            }

            calculated_character_size++;
            assert(calculated_character_size <= CHAR_CAPACITY);
        }
    }
}

void adjust_screen_text_area(Editor *editor) {
    float w, h;
    get_text_area_size(editor, &w, &h);
    if (w < viewport_size[0] * viewport_scale)
        viewport_pos[0] = 0;
    else if ((viewport_pos[0] + viewport_size[0]) * viewport_scale > w)
        viewport_pos[0] = w / viewport_scale - viewport_size[0];
    else if (viewport_pos[0] < 0)
        viewport_pos[0] = 0;
    
    if (h < viewport_size[1] * viewport_scale)
        viewport_pos[1] = 0;
    else if ((viewport_pos[1] + viewport_size[1]) * viewport_scale > h)
        viewport_pos[1] = h / viewport_scale - viewport_size[1];
    else if (viewport_pos[1] < 0)
        viewport_pos[1] = 0;
}

void adjust_screen_cursor(Editor *editor) {
    float x, y;
    get_cursor_pos(editor, &x, &y);
    if (x < viewport_pos[0] * viewport_scale)
        viewport_pos[0] = x / viewport_scale;
    if (x + cursor_size[0] > (viewport_pos[0] + viewport_size[0]) * viewport_scale)
        viewport_pos[0] = (x + cursor_size[0]) / viewport_scale - viewport_size[0];
        
    if (y < viewport_pos[1] * viewport_scale)
        viewport_pos[1] = y / viewport_scale;
    if (y + cursor_size[1] > (viewport_pos[1] + viewport_size[1]) * viewport_scale)
        viewport_pos[1] = (y + cursor_size[1]) / viewport_scale - viewport_size[1];
    adjust_screen_text_area(editor);
}
void cursor_up(Editor *editor) {
    if (editor->cursor_y == 0)
        editor->cursor_x = 0;
    else
        editor->cursor_y -= 1;

    adjust_screen_cursor(editor);
}
void cursor_down(Editor *editor) {
    if (editor->cursor_y == editor->num_lines - 1) {
        size_t line_start = editor->lines[editor->cursor_y].start;
        size_t line_size = editor->lines[editor->cursor_y + 1].start - line_start;
        editor->cursor_x = line_size - 1;
    } else
        editor->cursor_y += 1;

    adjust_screen_cursor(editor);
}
void cursor_left(Editor *editor) {
    size_t line_start = editor->lines[editor->cursor_y].start;
    size_t line_size = editor->lines[editor->cursor_y + 1].start - line_start;
    if ((editor->cursor_x == 0 || line_size == 1) && editor->cursor_y > 0) { // at line start, wrap to previous line
    
        editor->cursor_y--;
        line_start = editor->lines[editor->cursor_y].start;
        line_size = editor->lines[editor->cursor_y + 1].start - line_start;
        editor->cursor_x = line_size - 1;
    } else if (editor->cursor_x > 0 && line_size > 1) {
        if (editor->cursor_x >= line_size)
            editor->cursor_x = line_size - 2;
        else {
            editor->cursor_x--;
        }
    }

    adjust_screen_cursor(editor);
}
void cursor_right(Editor *editor) {
    size_t line_start = editor->lines[editor->cursor_y].start;
    size_t line_size = editor->lines[editor->cursor_y + 1].start - line_start;
    if ((editor->cursor_x >= line_size - 1 || line_size == 1) && editor->cursor_y < editor->num_lines - 1) { // at line end, wrap to next line
        editor->cursor_y++;
        editor->cursor_x = 0;
    } else if (editor->cursor_x < line_size - 1 && line_size > 1) {
        editor->cursor_x++;
    }
    adjust_screen_cursor(editor);
}

void cursor_home(Editor *editor) {
    editor->cursor_x = 0;
    adjust_screen_cursor(editor);
}

void cursor_end(Editor *editor) {
    size_t line_start = editor->lines[editor->cursor_y].start;
    size_t line_size = editor->lines[editor->cursor_y + 1].start - line_start;
    editor->cursor_x = line_size - 1;
    adjust_screen_cursor(editor);
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
    for (size_t i = e->cursor_y + 1; i <= e->num_lines; i++)
        e->lines[i].start++;
    
    e->lines[e->cursor_y].render_length += char_params[(size_t)ch].advance_x;
    if (ch == '\n') {
        assert(e->num_lines < MAX_LINES);
        memmove(e->lines + e->cursor_y + 2, e->lines + e->cursor_y + 1, (e->num_lines - e->cursor_y) * sizeof(e->lines[0]));
        e->lines[e->cursor_y + 1].start = pos + 1;
        // calculate render length before
        float current_length = 0;
        for (size_t i = e->lines[e->cursor_y].start; i <= pos; i++) {
            current_length += char_params[(size_t)e->text[i]].advance_x;
        }
        e->lines[e->cursor_y + 1].render_length = e->lines[e->cursor_y].render_length - current_length;
        e->lines[e->cursor_y].render_length = current_length;

        e->num_lines++;
        e->cursor_x = 0;
        e->cursor_y++;
    } else {
        e->cursor_x = pos - e->lines[e->cursor_y].start + 1;
    }
    adjust_screen_cursor(e);
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
        e->lines[i].start--;
    }
    e->lines[e->cursor_y].render_length -= char_params[(size_t)deleted].advance_x;
    if (deleted == '\n') {
        e->num_lines--;
        e->lines[e->cursor_y].render_length += e->lines[e->cursor_y + 1].render_length;
        memmove(e->lines + e->cursor_y + 1, e->lines + e->cursor_y + 2, (e->num_lines - e->cursor_y) * sizeof(e->lines[0]));
    }
    e->cursor_x = pos - e->lines[e->cursor_y].start;
    adjust_screen_text_area(e);
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
        e->lines[i].start--;
    }

    e->lines[e->cursor_y].render_length -= char_params[(size_t)deleted].advance_x;
    if (deleted == '\n') {
        e->num_lines--;
        e->lines[e->cursor_y - 1].render_length += e->lines[e->cursor_y].render_length;
        memmove(e->lines + e->cursor_y, e->lines + e->cursor_y + 1, (e->num_lines - e->cursor_y + 1) * sizeof(e->lines[0]));
        e->cursor_y--;
        e->cursor_x = pos - e->lines[e->cursor_y].start;
    } else {
        e->cursor_x--;
    }
    adjust_screen_cursor(e);
}

void insert_spaces(Editor *editor) {
    size_t effective_left = get_cursor_index(editor) - editor->lines[editor->cursor_y].start;
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
    (void)area;
    (void)keycode;
    (void)event_controller;
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
    layout_updated = true;
    

    struct timeval time;
    gettimeofday(&time, NULL);
    time.tv_sec -= start_sec;
    last_edit = (float)time.tv_usec / 1000000 + time.tv_sec;
    return TRUE;
}

gboolean handle_scroll(
    GtkEventControllerScroll* self,
    gdouble dx,
    gdouble dy,
    gpointer user_data
) {
    (void)self;
    (void)user_data;
    viewport_velocity[0] += SCROLL_SPEED * dx;
    viewport_velocity[1] += SCROLL_SPEED * dy;
    return TRUE;
}

void handle_mouse_down (
    GtkGesture* self,
    GdkEventSequence* sequence,
    gpointer user_data
) {
    (void)user_data;
    double x, y;
    gtk_gesture_get_point(self, sequence, &x, &y);
    x = (x + viewport_pos[0]) * viewport_scale;
    y = (y + viewport_pos[1] - CURSOR_OFFSET_Y) * viewport_scale;
    main_editor.cursor_y = (size_t)(y / line_height);
    if (main_editor.cursor_y >= main_editor.num_lines)
        main_editor.cursor_y = main_editor.num_lines - 1;
    else if (y / line_height < 0)
        main_editor.cursor_y = 0;

    if (x <= 0) {
        main_editor.cursor_x = 0;
    } else {
        // TODO: replace with binary search, although improvement would be insignificant
        float render_length = 0;
        bool rendered = false;
        for (size_t i = main_editor.lines[main_editor.cursor_y].start; i < main_editor.lines[main_editor.cursor_y + 1].start; i++) {
            FT_Long prevhalf = char_params[(size_t)main_editor.text[i]].advance_x / 2;
            FT_Long nexthalf = char_params[(size_t)main_editor.text[i]].advance_x - prevhalf;
            render_length += prevhalf;
            if (render_length >= x) {
                main_editor.cursor_x = i -  main_editor.lines[main_editor.cursor_y].start;
                rendered = true;
                break;
            }
            render_length += nexthalf;
        }
        if (!rendered) {
            main_editor.cursor_x = main_editor.lines[main_editor.cursor_y + 1].start - main_editor.lines[main_editor.cursor_y].start - 1; 
        }
    }
    adjust_screen_cursor(&main_editor);
    layout_updated = true;
    struct timeval time;
    gettimeofday(&time, NULL);
    time.tv_sec -= start_sec;
    last_edit = (float)time.tv_usec / 1000000 + time.tv_sec;
}
void handle_mouse_up (
    GtkGesture* self,
    GdkEventSequence* sequence,
    gpointer user_data
) {

    (void)user_data;
    double x, y;
    gtk_gesture_get_point(self, sequence, &x, &y);
}