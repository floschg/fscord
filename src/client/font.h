#ifndef FONT_H
#define FONT_H

#include <basic/basic.h>
#include <basic/string32.h>

#include <stb/stb_truetype.h>

typedef struct {
    i32 width;
    i32 height;
    union {
        u32 *rgba;
        u8 *grayscale;
    } data;
} Bitmap;

typedef struct {
    f32 xoff;
    f32 yoff;
    f32 advance_width;
    Bitmap bitmap;
} Glyph;

typedef struct {
    Arena arena;
    stbtt_fontinfo info;
    f32 scale;
    i32 baseline;
    i32 y_advance;
    Glyph glyphs[96];
} Font;


b32     font_init(Font *font, char *path, int font_size);
Glyph*  font_get_glyph(Font *font, u32 codepoint);
f32     font_get_string32_width(Font *font, String32 *str);
f32     font_get_string32_cursor_xoff(Font *font, String32 *str, u32 cursor);
f32     font_get_height(Font *font);

#endif // FONT_H
