#ifndef _FREETYPE_H
#define _FREETYPE_H

#include <ft2build.h>
#include FT_FREETYPE_H
#define FONT "fonts/NotoSans-Regular.ttf"

extern FT_Library lib;
extern FT_Face face;

void freetype_init(void);
int get_glyph_slot(char ch, unsigned int *width, unsigned int *height);
#endif // _FREETYPE_H