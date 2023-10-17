#include "editor.h"
#include "render.h"
#include "freetype.h"
Vertex calculated_characters[CHAR_CAPACITY];
size_t calculated_character_size = 0;



void init_editor(Editor *self) {
    unsigned int accumulated_width = 0;
    int line_height = 60;
    int letter_spacing = 5;
    char word[] = "Hello, world!";
    for (char *ptr = word; *ptr != '\0'; ++ptr) {
        unsigned int width, height;
        if (get_glyph_slot(*ptr, &width, &height)) {
            printf("Could not get font width for character %c!\n", *ptr);
            continue;
        }
        printf("%d %d\n", width, height);
        calculated_characters[calculated_character_size] = (Vertex) {
            .ch = (float)*ptr,
            .color = {(float)(*ptr % 5) / 5, (float)(*ptr % 7) / 7, (float)(*ptr % 11) / 11},
            .pos = {accumulated_width, line_height - height},
            .size = {width, height}
        };
        accumulated_width += width + letter_spacing;
        calculated_character_size++;
    }
}