#include "editor.h"
#include "render.h"
#include "freetype.h"
Vertex calculated_characters[CHAR_CAPACITY];
size_t calculated_character_size = 0;



void init_editor() {
    unsigned int accumulated_width = 0, accumulated_height = 0;
    int line_height = 20;
    int letter_spacing = 0;
    char word[] = "#include <stdio.h>\n\nint main() {\n    printf(\"Hello world!\\n\");\n    return 0;\n}\n";
    for (char *ptr = word; *ptr != '\0'; ++ptr) {
        if (*ptr == '\n') {
            accumulated_height += line_height;
            accumulated_width = 0;
            continue;
        }
        size_t index = (size_t) *ptr;
        calculated_characters[calculated_character_size] = (Vertex) {
            .uv_offset_x = char_params[index].offset,
            .uv_size = {char_params[index].uv_width, char_params[index].uv_height},
            .color = {1.0, 1.0, 1.0},
            .pos = {accumulated_width + char_params[index].bitmap_left, accumulated_height + line_height - char_params[index].bitmap_top},
            .size = {char_params[index].width, char_params[index].height}
        };
        accumulated_width += char_params[index].advance_x + letter_spacing;
        calculated_character_size++;
    }
}