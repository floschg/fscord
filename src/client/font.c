#include "basic/basic.h"
#include <os/os.h>
#include <client/font.h>
#include <assert.h>

#include <stb/stb_truetype.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <string.h>
#include <math.h>

f32
font_get_string32_cursor_xoff(Font *font, String32 *str, u32 cursor) {
    f32 xoff = 0.0f;
    u32 x = 0;
    while (x < cursor) {
        Glyph *glyph = font_get_glyph(font, str->codepoints[x]);
        xoff += glyph->advance_width;
        x++;
    }
    return xoff;
}

f32
font_get_string32_width(Font *font, String32 *str)
{
    // @Todo: also consider left_side_bearing and kerning?
    f32 width = 0.0f;
    for (u64 i = 0; i < str->len; i++) {
        Glyph *glyph = font_get_glyph(font, str->codepoints[i]);
        width += glyph->advance_width;
    }
    return width;
}

f32
font_get_height(Font *font)
{
    return font->y_advance;
}

Glyph *
font_get_glyph(Font *font, u32 codepoint)
{
    assert(codepoint >= ' ' && codepoint <= '~');
    i32 index = codepoint - ' ';
    Glyph *glyph = &font->glyphs[index];
    return glyph;
}

b32
font_init(Font *font, char *path, int font_size)
{
    arena_init(&font->arena, MEBIBYTES(4));

    size_t len;
    u8 *ttf = (u8*)os_file_read_as_string(&font->arena, path, &len);
    if (!ttf) {
        arena_deinit(&font->arena);
        return false;
    }


    // set font info
    if (!stbtt_InitFont(&font->info, ttf, 0))
    {
        printf("stbtt_InitFont failed\n");
        arena_deinit(&font->arena);
        return false;
    }


    // set scale
    f32 scale = stbtt_ScaleForPixelHeight(&font->info, font_size);


    // get font vmetrics
    i32 baseline, ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
    baseline = scale * -descent;
    ascent   = scale * ascent;
    descent  = scale * descent;
    line_gap = scale * line_gap;


    font->scale = scale;
    font->baseline = baseline;
    font->y_advance = ascent - descent + line_gap;


    // make glyphs
    u32 glyph_count = '~' - ' ' + 1;
    for (u32 it = 0; it < glyph_count; ++it)
    {
        Glyph *glyph = font->glyphs + it;
        char c = it + ' ';


        int bbx0, bby0, bbx1, bby1;
        stbtt_GetCodepointBitmapBox(&font->info, c, scale, scale, &bbx0, &bby0, &bbx1, &bby1);
        int width = bbx1 - bbx0;
        int height = bby1 - bby0;


        u8 *bitmap_correct = arena_push(&font->arena, width * height);
        u32 arena_save_pos = font->arena.size_used;
        u8 *bitmap_flipped = arena_push(&font->arena, width * height);


        stbtt_MakeCodepointBitmap(&font->info, bitmap_flipped, width, height, width, scale, scale, c);

        // @Speed: Adjust projection and don't execute this at all?
        u8 *dest = bitmap_correct;
        for (u32 y = 0; y < height; ++y)
        {
            u8 *src = bitmap_flipped + (height-1-y)*width;
            for (u32 x = 0; x < width; ++x)
            {
                *dest++ = *src++;
            }
        }

        i32 advance_width;
        i32 left_side_bearing;
        stbtt_GetCodepointHMetrics(&font->info, c, &advance_width, &left_side_bearing);
        advance_width = scale * advance_width;
        left_side_bearing = scale * left_side_bearing;


        glyph->xoff = scale * bbx0 + left_side_bearing;
        glyph->yoff = -bby1;
        glyph->advance_width = advance_width;
        glyph->bitmap.width  = width;
        glyph->bitmap.height = height;
        glyph->bitmap.data.alpha = bitmap_correct;
        font->arena.size_used = arena_save_pos;
    }

    return true;
}

