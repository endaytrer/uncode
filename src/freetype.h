#ifndef _FREETYPE_H
#define _FREETYPE_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <GL/glew.h>
#include <GL/gl.h>


#define FONT "fonts/NotoSansMono-Regular.ttf"
#define FONT_SIZE_PT 14
#define PPI 72

#define FONT_TEXTURE GL_TEXTURE0

extern FT_Library lib;
extern FT_Face face;

typedef struct {
    float offset;
    float uv_width;
    float uv_height;
    unsigned long width, height;
    FT_Int bitmap_left;
    FT_Int bitmap_top;
    FT_Long advance_x;
} CharParams;

extern CharParams char_params[UCHAR_MAX + 1];

GLuint freetype_init(void);
int get_glyph_slot(char ch, unsigned int *width, unsigned int *height);
#endif // _FREETYPE_H