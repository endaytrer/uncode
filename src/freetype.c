#include "freetype.h"

FT_Library lib;
FT_Face face;

CharParams char_params[UCHAR_MAX + 1];

GLuint freetype_init(void) {
    if (FT_Init_FreeType(&lib)) {
        fprintf(stderr, "Could not init FreeType Library!\n");
        exit(-1);
    }
    if (FT_New_Face(lib, FONT, 0, &face)) {
        fprintf(stderr, "Could not load font %s!\n", FONT);
        exit(-1);
    }

    if (FT_Set_Char_Size(face, 0, FONT_SIZE_PT * 64, PPI, PPI)) {
        fprintf(stderr, "Could not set char size\n");
        exit(-1);
    }

    // put ascii printable character into a altas.
    // first, get size
    unsigned int width = 0, height = 0;
    for (int c = 32; c <= UCHAR_MAX; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            fprintf(stderr, "Could not load char %d\n", c);
            continue;
        }

        if (face->glyph->bitmap.rows > height)
            height = face->glyph->bitmap.rows;
        width += face->glyph->bitmap.width;
    }
    
    // allocate texture based on size
    GLuint tex;
    glActiveTexture(FONT_TEXTURE);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // one byte per pixel
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);

    unsigned int temp_offset = 0;
    for (int c = 32; c <= UCHAR_MAX; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            continue;
        }
        char_params[c].offset = (float)temp_offset / width;
        char_params[c].width = face->glyph->bitmap.width;
        char_params[c].height = face->glyph->bitmap.rows;
        char_params[c].uv_width = (float)face->glyph->bitmap.width / width;
        char_params[c].uv_height = (float)face->glyph->bitmap.rows / height;
        char_params[c].bitmap_left = face->glyph->bitmap_left;
        char_params[c].bitmap_top = face->glyph->bitmap_top;
        char_params[c].advance_x = face->glyph->advance.x >> 6;
        glTexSubImage2D(GL_TEXTURE_2D, 0, temp_offset, 0, face->glyph->bitmap.width, face->glyph->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        temp_offset += face->glyph->bitmap.width;
    }

    return tex;
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