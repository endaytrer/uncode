#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <math.h>

#include "editor.h"
#include "render.h"
#include "freetype.h"
Glyph chars[MAX_CHARS];
size_t num_chars = 0;


Glyph overchars[MAX_OVERCHARS];
size_t num_overchars = 0;

Rect rects[MAX_RECTS];
size_t num_rects = 0;

int line_height;

float cursor_size[2];
float cursor_color[4] = CURSOR_COLOR;

int viewport_size[2];
float viewport_pos[2] = {0, 0};
float viewport_velocity[2] = {0, 0};

int viewport_scale = 2;
int prev_viewport_size_hash = 0;

int hash_size(void) {
    return viewport_size[0] * 28367 + viewport_size[1];
}
Editor main_editor = {0};

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
    editor->cursor.x = 0;
    editor->cursor.y = 0;

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
size_t get_cursor_index(Editor *editor, Cursor *cursor) {
    assert(cursor->y < editor->num_lines);

    size_t line_start = editor->lines[cursor->y].start;
    size_t line_size = editor->lines[cursor->y + 1].start - line_start;
    if (cursor->x >= line_size) return line_start + line_size - 1;
    return line_start + cursor->x;
}
size_t get_effective_left(Editor *editor, Cursor *cursor) {
    size_t line_start = editor->lines[cursor->y].start;
    size_t line_size = editor->lines[cursor->y + 1].start - line_start;
    if (cursor->x >= line_size) return line_size - 1;
    return cursor->x;
}

void get_cursor_pos(Editor *editor, Cursor *cursor, float *x, float *y) {
    *y = cursor->y * line_height + CURSOR_OFFSET_Y * viewport_scale;
    size_t line_start = editor->lines[cursor->y].start;
    size_t effective_left = get_effective_left(editor, cursor);
    *x = MARGIN_LEFT * viewport_scale;
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
    *w += MARGIN_LEFT * viewport_scale;
}

/**
 * return true if they are different
 */
bool get_s1_s2(Editor *e, Cursor **s1, Cursor **s2) {
    Cursor *s_1, *s_2;
    bool ans = true;
    if (e->selection_start.y == e->cursor.y) {
        size_t cursor_x = get_effective_left(e, &e->cursor);
        if (e->selection_start.x < cursor_x) {
            s_1 = &e->selection_start;
            s_2 = &e->cursor;
        } else {
            s_1 = &e->cursor;
            s_2 = &e->selection_start;
            if (e->selection_start.x == cursor_x)
                ans = false;
        }
    } else {
        if (e->selection_start.y < e->cursor.y) {
            s_1 = &e->selection_start;
            s_2 = &e->cursor;
        } else {
            s_1 = &e->cursor;
            s_2 = &e->selection_start;
        }
    }
    *s1 = s_1;
    *s2 = s_2;
    return ans;
}

bool layout_updated = true;
void calculate(Editor *editor, float time_since_key_pressed_sec) {

    line_height = viewport_scale * ((FONT_SIZE_PT * PPI) >> 6);
    cursor_size[0] = CURSOR_WIDTH * viewport_scale;
    cursor_size[1] = line_height;
    float cursor_x, cursor_y;
    get_cursor_pos(editor, &editor->cursor, &cursor_x, &cursor_y);


    { // calculate cursor every time to avoid out-dated cursor

        // calculate blinking color;
        const float T = 1.5f;
        const float PI = 3.14159265f;
        const float STRENGTH = 20.0f;
        const float OMEGA = 2.0 * PI / T;

        float alpha = cosf(OMEGA * time_since_key_pressed_sec);
        alpha = 1.0 / (1.0 + expf(-STRENGTH * alpha));
        // we need render cursor above rect of current line, but below line number region, so make it index 1;

        rects[1] = (Rect) {
            .size = {cursor_size[0], cursor_size[1]},
            .color = {cursor_color[0], cursor_color[1], cursor_color[2], alpha},
            .position = {cursor_x, cursor_y}
        };
    }
    

    int hash = hash_size();
    if (!layout_updated && hash == prev_viewport_size_hash) return;
    prev_viewport_size_hash = hash;
    layout_updated = false;

    num_rects = 2;
    num_chars = 0;
    num_overchars = 0;
    if (editor->size == 0) return;
    // rect of current line

    rects[0] = (Rect) {
        .size = {(viewport_size[0] - MARGIN_LEFT + NUMBER_MARGIN_RIGHT) * viewport_scale, line_height},
        .color = CURRENT_LINE_COLOR,
        .position = {(viewport_pos[0] + MARGIN_LEFT - NUMBER_MARGIN_RIGHT) * viewport_scale, cursor_y}
    };

    // rect of selections
    if (editor->mode == Selection) {
        // inline selection
        Cursor *s_1, *s_2;
        float s1x, s2x;
        if (editor->selection_start.y == editor->cursor.y) {
            float sy;
            size_t x_start = get_effective_left(editor, &editor->selection_start);
            size_t x_end = get_effective_left(editor, &editor->cursor);
            bool flag = true;
            if (x_start < x_end) {
                s_1 = &editor->selection_start;
                get_cursor_pos(editor, s_1, &s1x, &sy);
                s_2 = &editor->cursor;
                s2x = cursor_x;
            } else if (x_start > x_end) {
                s_1 = &editor->cursor;
                s1x = cursor_x;
                s_2 = &editor->selection_start;
                get_cursor_pos(editor, s_2, &s2x, &sy);
            } else {
                flag = false;
            }
            if (flag) {
                rects[num_rects] = (Rect) {
                    .size = {s2x - s1x, line_height},
                    .color = SELECTION_COLOR,
                    .position = {s1x, cursor_y}
                };

                if (rects[num_rects].position[0] <= (viewport_pos[0] + viewport_size[0]) * viewport_scale &&
                    rects[num_rects].position[0] + rects[num_rects].size[0] >= viewport_pos[0] * viewport_scale &&
                    rects[num_rects].position[1] <= (viewport_pos[1] + viewport_size[1]) * viewport_scale &&
                    rects[num_rects].position[1] + rects[num_rects].size[1] >= viewport_pos[1] * viewport_scale
                    ) {
                    num_rects++;
                }
            }
        } else {
            float s1y, s2y;
            if (editor->selection_start.y < editor->cursor.y) {
                s_1 = &editor->selection_start;
                get_cursor_pos(editor, s_1, &s1x, &s1y);
                s_2 = &editor->cursor;
                s2x = cursor_x;
                s2y = cursor_y;
            } else {
                s_1 = &editor->cursor;
                s1x = cursor_x;
                s1y = cursor_y;
                s_2 = &editor->selection_start;
                get_cursor_pos(editor, s_2, &s2x, &s2y);
            }
            rects[num_rects] = (Rect) {
                .size = {(viewport_size[0] + viewport_pos[0]) * viewport_scale - s1x, line_height},
                .color = SELECTION_COLOR,
                .position = {s1x, s1y}
            };

            if (rects[num_rects].position[0] <= (viewport_pos[0] + viewport_size[0]) * viewport_scale &&
                rects[num_rects].position[0] + rects[num_rects].size[0] >= viewport_pos[0] * viewport_scale &&
                rects[num_rects].position[1] <= (viewport_pos[1] + viewport_size[1]) * viewport_scale &&
                rects[num_rects].position[1] + rects[num_rects].size[1] >= viewport_pos[1] * viewport_scale
                ) {
                num_rects++;
            }
            for (size_t i = s_1->y + 1; i < s_2->y; i++) {
                rects[num_rects] = (Rect) {
                    .size = {(viewport_pos[0] + viewport_size[0]) * viewport_scale, line_height},
                    .color = SELECTION_COLOR,
                    .position = {MARGIN_LEFT * viewport_scale, i * line_height + CURSOR_OFFSET_Y * viewport_scale}
                }; 
                if (rects[num_rects].position[0] <= (viewport_pos[0] + viewport_size[0]) * viewport_scale &&
                    rects[num_rects].position[0] + rects[num_rects].size[0] >= viewport_pos[0] * viewport_scale &&
                    rects[num_rects].position[1] <= (viewport_pos[1] + viewport_size[1]) * viewport_scale &&
                    rects[num_rects].position[1] + rects[num_rects].size[1] >= viewport_pos[1] * viewport_scale
                    ) {
                    num_rects++;
                }
            }
            rects[num_rects] = (Rect) {
                .size = {s2x - (MARGIN_LEFT) * viewport_scale, line_height},
                .color = SELECTION_COLOR,
                .position = {MARGIN_LEFT * viewport_scale, s2y}
            };

            if (rects[num_rects].position[0] <= (viewport_pos[0] + viewport_size[0]) * viewport_scale &&
                rects[num_rects].position[0] + rects[num_rects].size[0] >= viewport_pos[0] * viewport_scale &&
                rects[num_rects].position[1] <= (viewport_pos[1] + viewport_size[1]) * viewport_scale &&
                rects[num_rects].position[1] + rects[num_rects].size[1] >= viewport_pos[1] * viewport_scale
                ) {
                num_rects++;
            }
        }
    }

    // rect of line numbers

    rects[num_rects++] = (Rect) {
        .size = {(MARGIN_LEFT - NUMBER_MARGIN_RIGHT) * viewport_scale, viewport_size[1] * viewport_scale},
        .color = NUMBER_BG_COLOR,
        .position = {viewport_pos[0] * viewport_scale, viewport_pos[1] * viewport_scale}
    };


    int start = viewport_pos[1] * viewport_scale / line_height - 2;
    size_t line = start < 0 ? 0 : start;
    size_t end = (viewport_pos[1] + viewport_size[1]) * viewport_scale / line_height + 1;
    if (editor->num_lines - 1 < end)
        end = editor->num_lines - 1;
    for (; line <= end; line++) {
        size_t accumulated_width = MARGIN_LEFT * viewport_scale;
        size_t top = line * line_height;
        for (size_t i = editor->lines[line].start; i < editor->lines[line + 1].start; i++) {

            size_t index = (size_t) editor->text[i];

            chars[num_chars] = (Glyph) {
                .uv_offset_x = char_params[index].offset,
                .uv_size = {char_params[index].uv_width, char_params[index].uv_height},
                .position = {accumulated_width + char_params[index].bitmap_left, top + line_height - char_params[index].bitmap_top},
                .size = {char_params[index].width, char_params[index].height},
                .fg_color = FG_COLOR
            };
            accumulated_width += char_params[index].advance_x;

            if (chars[num_chars].position[0] > (viewport_pos[0] + viewport_size[0]) * viewport_scale) {
                break;
            }
            if (chars[num_chars].position[0] + chars[num_chars].size[0] < viewport_pos[0] * viewport_scale ||
                chars[num_chars].position[1] > (viewport_pos[1] + viewport_size[1]) * viewport_scale ||
                chars[num_chars].position[1] + chars[num_chars].size[1] < viewport_pos[1] * viewport_scale
                ) {
                continue;
            }

            num_chars++;
            assert(num_chars <= MAX_CHARS);
        }
        char line_number[5]; // since max lines is 4096, use 5 bytes
        int n = snprintf(line_number, 5, "%ld", line + 1);
        size_t right = (MARGIN_LEFT - NUMBER_PADDING - NUMBER_MARGIN_RIGHT) * viewport_scale;
        for (int i = n - 1; i >= 0; i--) {
            size_t index = (size_t) line_number[i];

            overchars[num_overchars] = (Glyph) {
                .uv_offset_x = char_params[index].offset,
                .uv_size = {char_params[index].uv_width, char_params[index].uv_height},
                .position = {viewport_pos[0] * viewport_scale + right - char_params[index].advance_x + char_params[index].bitmap_left,
                            top + line_height - char_params[index].bitmap_top},
                .size = {char_params[index].width, char_params[index].height},
                .fg_color = NUMBER_COLOR
            };

            right -= char_params[index].advance_x;

            if (overchars[num_overchars].position[0] > (viewport_pos[0] + viewport_size[0]) * viewport_scale ||
                overchars[num_overchars].position[0] + overchars[num_overchars].size[0] < viewport_pos[0] * viewport_scale ||
                overchars[num_overchars].position[1] > (viewport_pos[1] + viewport_size[1]) * viewport_scale ||
                overchars[num_overchars].position[1] + overchars[num_overchars].size[1] < viewport_pos[1] * viewport_scale
                ) {
                continue;
            }

            if (line == editor->cursor.y) {
                const float cnc[] = CURRENT_NUMBER_COLOR;
                overchars[num_overchars].fg_color[0] = cnc[0];
                overchars[num_overchars].fg_color[1] = cnc[1];
                overchars[num_overchars].fg_color[2] = cnc[2];
            }

            num_overchars++;
            assert(num_overchars <= MAX_OVERCHARS);
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

void adjust_screen_cursor(Editor *editor, Cursor *cursor) {
    adjust_screen_text_area(editor);
    float x, y;
    get_cursor_pos(editor, cursor, &x, &y);
    if (x < (viewport_pos[0] + MARGIN_LEFT) * viewport_scale)
        viewport_pos[0] = x / viewport_scale - MARGIN_LEFT;
    if (x + cursor_size[0] > (viewport_pos[0] + viewport_size[0]) * viewport_scale)
        viewport_pos[0] = (x + cursor_size[0]) / viewport_scale - viewport_size[0];
        
    if (y < viewport_pos[1] * viewport_scale)
        viewport_pos[1] = y / viewport_scale;
    if (y + cursor_size[1] > (viewport_pos[1] + viewport_size[1]) * viewport_scale)
        viewport_pos[1] = (y + cursor_size[1]) / viewport_scale - viewport_size[1];
}
void cursor_up(Editor *editor) {
    if (editor->cursor.y == 0)
        editor->cursor.x = 0;
    else
        editor->cursor.y -= 1;

}
void cursor_down(Editor *editor) {
    if (editor->cursor.y == editor->num_lines - 1) {
        size_t line_start = editor->lines[editor->cursor.y].start;
        size_t line_size = editor->lines[editor->cursor.y + 1].start - line_start;
        editor->cursor.x = line_size - 1;
    } else
        editor->cursor.y += 1;

}
void cursor_left(Editor *editor) {
    size_t line_start = editor->lines[editor->cursor.y].start;
    size_t line_size = editor->lines[editor->cursor.y + 1].start - line_start;
    if ((editor->cursor.x == 0 || line_size == 1) && editor->cursor.y > 0) { // at line start, wrap to previous line
    
        editor->cursor.y--;
        line_start = editor->lines[editor->cursor.y].start;
        line_size = editor->lines[editor->cursor.y + 1].start - line_start;
        editor->cursor.x = line_size - 1;
    } else if (editor->cursor.x > 0 && line_size > 1) {
        if (editor->cursor.x >= line_size)
            editor->cursor.x = line_size - 2;
        else {
            editor->cursor.x--;
        }
    }
}
void cursor_right(Editor *editor) {
    size_t line_start = editor->lines[editor->cursor.y].start;
    size_t line_size = editor->lines[editor->cursor.y + 1].start - line_start;
    if ((editor->cursor.x >= line_size - 1 || line_size == 1) && editor->cursor.y < editor->num_lines - 1) { // at line end, wrap to next line
        editor->cursor.y++;
        editor->cursor.x = 0;
    } else if (editor->cursor.x < line_size - 1 && line_size > 1) {
        editor->cursor.x++;
    }
}

void cursor_home(Editor *editor) {
    editor->cursor.x = 0;
}

void cursor_end(Editor *editor) {
    size_t line_start = editor->lines[editor->cursor.y].start;
    size_t line_size = editor->lines[editor->cursor.y + 1].start - line_start;
    editor->cursor.x = line_size - 1;
}

void delete(Editor *e) {
    size_t pos = get_cursor_index(e, &e->cursor);
    if (pos == e->size - 1) return;
    if (e->size <= e->capacity / 2) {
        size_t new_cap = e->capacity / 2;
        e->text = realloc(e->text, new_cap);
        e->capacity = new_cap;
    } 

    e->size -= 1;
    char deleted = e->text[pos];
    memmove(e->text + pos, e->text + pos + 1, e->size - pos);

    for (size_t i = e->cursor.y + 1; i <= e->num_lines; i++) {
        e->lines[i].start--;
    }
    e->lines[e->cursor.y].render_length -= char_params[(size_t)deleted].advance_x;
    if (deleted == '\n') {
        e->num_lines--;
        e->lines[e->cursor.y].render_length += e->lines[e->cursor.y + 1].render_length;
        memmove(e->lines + e->cursor.y + 1, e->lines + e->cursor.y + 2, (e->num_lines - e->cursor.y) * sizeof(e->lines[0]));
    }
    e->cursor.x = pos - e->lines[e->cursor.y].start;
}

size_t get_selection_size(Editor *e) {
    Cursor *s_1, *s_2;

    if (get_s1_s2(e, &s_1, &s_2))
        return get_cursor_index(e, s_2) - get_cursor_index(e, s_1);
    else
        return 0;
}
void delete_selection(Editor *e) {
    Cursor *s_1, *s_2;
    e->cursor.x = get_effective_left(e, &e->cursor);
    if (get_s1_s2(e, &s_1, &s_2)) {
        size_t size = get_cursor_index(e, s_2) - get_cursor_index(e, s_1);
        // TODO: delete at once, will result in significant performance improvement
        e->cursor = *s_1;
        for (size_t i = 0; i < size; i++) {
            delete(e);
        }
    } 
}

void insert(Editor *e, char ch) {
    // Insert at cursor
    // if at selection, remove all content in selection;
    if (e->mode == Selection)
        delete_selection(e);

    size_t pos = get_cursor_index(e, &e->cursor);
    if (e->size == e->capacity) {
        size_t new_cap = e->capacity * 2;
        e->text = realloc(e->text, new_cap);
        e->capacity = new_cap;
    }
    memmove(e->text + pos + 1, e->text + pos, e->size - pos);
    e->text[pos] = ch;
    e->size += 1;
    // recalculate every line start after current line to +1;
    for (size_t i = e->cursor.y + 1; i <= e->num_lines; i++)
        e->lines[i].start++;
    
    e->lines[e->cursor.y].render_length += char_params[(size_t)ch].advance_x;
    if (ch == '\n') {
        assert(e->num_lines < MAX_LINES);
        memmove(e->lines + e->cursor.y + 2, e->lines + e->cursor.y + 1, (e->num_lines - e->cursor.y) * sizeof(e->lines[0]));
        e->lines[e->cursor.y + 1].start = pos + 1;
        // calculate render length before
        float current_length = 0;
        for (size_t i = e->lines[e->cursor.y].start; i <= pos; i++) {
            current_length += char_params[(size_t)e->text[i]].advance_x;
        }
        e->lines[e->cursor.y + 1].render_length = e->lines[e->cursor.y].render_length - current_length;
        e->lines[e->cursor.y].render_length = current_length;

        e->num_lines++;
        e->cursor.x = 0;
        e->cursor.y++;
    } else {
        e->cursor.x = pos - e->lines[e->cursor.y].start + 1;
    }
    // since inserting character introduces actual mouse motion, adjust screen cursor as well.
    adjust_screen_cursor(e, &e->cursor);
}

void handle_delete(Editor *e) {
    if (e->mode == Selection)
        delete_selection(e);
    else
        delete(e);
}

void backspace(Editor *e) {
    if (e->mode == Selection) {
        delete_selection(e);
        return;
    }

    size_t pos = get_cursor_index(e, &e->cursor);
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

    for (size_t i = e->cursor.y + 1; i <= e->num_lines; i++) {
        e->lines[i].start--;
    }

    e->lines[e->cursor.y].render_length -= char_params[(size_t)deleted].advance_x;
    if (deleted == '\n') {
        e->num_lines--;
        e->lines[e->cursor.y - 1].render_length += e->lines[e->cursor.y].render_length;
        memmove(e->lines + e->cursor.y, e->lines + e->cursor.y + 1, (e->num_lines - e->cursor.y + 1) * sizeof(e->lines[0]));
        e->cursor.y--;
        e->cursor.x = pos - e->lines[e->cursor.y].start;
    } else {
        e->cursor.x--;
    }
}

void insert_spaces(Editor *editor) {
    size_t effective_left = get_effective_left(editor, &editor->cursor);
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
    if (main_editor.mode == Normal) {
        main_editor.selection_start = main_editor.cursor;
        main_editor.selection_start.x = get_effective_left(&main_editor, &main_editor.selection_start);
    }

    bool mouse_motion = false;
    switch (keyval) {
        case GDK_KEY_Up:
            if (main_editor.mode == Selection && !(state & GDK_SHIFT_MASK)) {
                Cursor *s1, *s2;
                main_editor.cursor.x = get_effective_left(&main_editor, &main_editor.cursor);
                get_s1_s2(&main_editor, &s1, &s2);
                main_editor.cursor = *s1;
            } 
            cursor_up(&main_editor);
            mouse_motion = true;
            break;

        case GDK_KEY_Down:
            if (main_editor.mode == Selection && !(state & GDK_SHIFT_MASK)) {
                Cursor *s1, *s2;
                main_editor.cursor.x = get_effective_left(&main_editor, &main_editor.cursor);
                get_s1_s2(&main_editor, &s1, &s2);
                main_editor.cursor = *s2;
            } 
            cursor_down(&main_editor);
            mouse_motion = true;
            break;

        case GDK_KEY_Left:
            if (main_editor.mode == Selection && !(state & GDK_SHIFT_MASK)) {
                Cursor *s1, *s2;
                main_editor.cursor.x = get_effective_left(&main_editor, &main_editor.cursor);
                get_s1_s2(&main_editor, &s1, &s2);
                main_editor.cursor = *s1;
                break;
            } else
                cursor_left(&main_editor);
            mouse_motion = true;
            break;
        
        case GDK_KEY_Right:
            if (main_editor.mode == Selection && !(state & GDK_SHIFT_MASK)) {
                Cursor *s1, *s2;
                main_editor.cursor.x = get_effective_left(&main_editor, &main_editor.cursor);
                get_s1_s2(&main_editor, &s1, &s2);
                main_editor.cursor = *s2;
                break;
            } else
                cursor_right(&main_editor);
            mouse_motion = true;
            break;

        case GDK_KEY_Home:
            cursor_home(&main_editor);
            mouse_motion = true;
            break;
        
        case GDK_KEY_End:
            cursor_end(&main_editor);
            mouse_motion = true;
            break;
        
        case GDK_KEY_Return:
            insert(&main_editor, '\n');
            mouse_motion = true;
            break;

        case GDK_KEY_BackSpace:
            backspace(&main_editor);
            mouse_motion = true;
            break;

        case GDK_KEY_Delete:
            handle_delete(&main_editor);
            break;

        case GDK_KEY_Tab:
            insert_spaces(&main_editor);
            mouse_motion = true;
            break;

        default:
            if (keyval <= UCHAR_MAX)
                insert(&main_editor, (char)keyval);
            else
                return TRUE;
    }
    if (mouse_motion) {
        if ((state & GDK_SHIFT_MASK) && get_selection_size(&main_editor)) {
            main_editor.mode = Selection;
        } else {
            main_editor.mode = Normal;
        }
        adjust_screen_cursor(&main_editor, &main_editor.cursor);
    } else {
        main_editor.mode = Normal;
        adjust_screen_text_area(&main_editor);
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

void pos_to_cursor_xy(double pos_x, double pos_y, Editor *editor, Cursor *cursor) {
    int y = (int)(pos_y / line_height);
    cursor->y = y >= 0 ? (size_t)y : 0;
    if (cursor->y >= editor->num_lines)
        cursor->y = editor->num_lines - 1;

    if (pos_x <= 0) {
        cursor->x = 0;
    } else {
        // TODO: replace with binary search, although improvement would be insignificant
        float char_max_pos = MARGIN_LEFT * viewport_scale;
        bool rendered = false;
        for (size_t i = editor->lines[cursor->y].start; i < editor->lines[cursor->y + 1].start; i++) {
            FT_Long prevhalf = char_params[(size_t)editor->text[i]].advance_x / 2;
            FT_Long nexthalf = char_params[(size_t)editor->text[i]].advance_x - prevhalf;
            char_max_pos += prevhalf;
            if (char_max_pos >= pos_x) {
                cursor->x = i -  editor->lines[cursor->y].start;
                rendered = true;
                break;
            }
            char_max_pos += nexthalf;
        }
        if (!rendered) {
            cursor->x = editor->lines[cursor->y + 1].start - editor->lines[cursor->y].start - 1; 
        }
    }
}

bool mouse_pressed = false;
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
    pos_to_cursor_xy(x, y, &main_editor, &main_editor.selection_start);
    main_editor.cursor = main_editor.selection_start;

    adjust_screen_cursor(&main_editor, &main_editor.selection_start);
    layout_updated = true;
    struct timeval time;
    gettimeofday(&time, NULL);
    mouse_pressed = true;

    time.tv_sec -= start_sec;
    last_edit = (float)time.tv_usec / 1000000 + time.tv_sec;
}
void handle_mouse_up (
    GtkGesture* self,
    GdkEventSequence* sequence,
    gpointer user_data
) {
    (void)self;
    (void)user_data;
    (void)sequence;

    mouse_pressed = false;
    layout_updated = true;
    struct timeval time;
    gettimeofday(&time, NULL);

    time.tv_sec -= start_sec;
    last_edit = (float)time.tv_usec / 1000000 + time.tv_sec;
}

void handle_mouse_move(
    GtkEventControllerMotion* self,
    gdouble x,
    gdouble y,
    gpointer user_data
) {
    (void)self;
    (void)user_data;

    if (!mouse_pressed) return;

    x = (x + viewport_pos[0]) * viewport_scale;
    y = (y + viewport_pos[1] - CURSOR_OFFSET_Y) * viewport_scale;

    pos_to_cursor_xy(x, y, &main_editor, &main_editor.cursor);
    if (get_selection_size(&main_editor)) {
        main_editor.mode = Selection;
    } else {
        main_editor.mode = Normal;
    }
    layout_updated = true;
    struct timeval time;
    gettimeofday(&time, NULL);

    time.tv_sec -= start_sec;
    last_edit = (float)time.tv_usec / 1000000 + time.tv_sec;
}