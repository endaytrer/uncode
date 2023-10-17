#include "freetype.h"

FT_Library lib;
FT_Face face;

void freetype_init(void) {
    if (FT_Init_FreeType(&lib)) {
        fprintf(stderr, "Could not init FreeType Library!\n");
        exit(-1);
    }
    if (FT_New_Face(lib, FONT, 0, &face)) {
        fprintf(stderr, "Could not load font %s!\n", FONT);
        exit(-1);
    }
    printf("Num glyphs: %ld\n", face->num_glyphs);

    if (FT_Set_Char_Size(face, 0, 16 * 64, 300, 300)) {
        fprintf(stderr, "Could not set char size\n");
        exit(-1);
    }
}

int get_glyph_slot(char ch, unsigned int *width, unsigned int *height) {
    FT_Error error;
    if ((error = FT_Load_Char(face, ch, FT_LOAD_RENDER))) {
        return error;
    }
    *width = face->glyph->bitmap.width;
    *height = face->glyph->bitmap.rows;
    return 0;
}