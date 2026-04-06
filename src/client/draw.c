#include <os/os.h>
#include <basic/math.h>
#include <client/draw.h>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>


static OSOffscreenBuffer *s_ob;


void
init_drawing(OSOffscreenBuffer *ob)
{
    s_ob = ob;
}

void
draw_rectf32(RectF32 rect, V3F32 color)
{
    i32 xmin = f32_round_to_i32(rect.x0);
    i32 ymin = f32_round_to_i32(rect.y0);
    i32 xmax = f32_round_to_i32(rect.x1);
    i32 ymax = f32_round_to_i32(rect.y1);

    if (xmin < 0) {
        xmin = 0;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (xmax >= s_ob->width) {
        xmax = s_ob->width - 1;
    }
    if (ymax >= s_ob->height) {
        ymax = s_ob->height - 1;
    }

    u8 rshift = s_ob->red_shift;
    u8 gshift = s_ob->green_shift;
    u8 bshift = s_ob->blue_shift;

    f32 r = color.x * 255.0;
    f32 g = color.y * 255.0;
    f32 b = color.z * 255.0;
    u32 val = (u32)r << rshift | (u32)g << gshift | (u32)b << bshift;

    for (i32 y = ymin; y <= ymax; y++) {
        u32 *pixel = s_ob->pixels + y * s_ob->width + xmin;
        for (i32 x = xmin; x <= xmax; x++) {
            *pixel++ = val;
        }
    }
}

void
draw_mono_bitmap(V2F32 pos, Bitmap *bitmap, V3F32 color)
{
    i32 x0 = f32_round_to_i32(pos.x);
    i32 y0 = f32_round_to_i32(pos.y);
    i32 x1 = x0 + bitmap->width - 1;
    i32 y1 = y0 + bitmap->height - 1;

    i32 cut_left = 0;
    i32 cut_bot = 0;

    if (x0 < 0) {
        cut_left = x0;
        x0 = 0;
    }
    if (y0 < 0) {
        cut_bot = y0;
        y0 = 0;
    }
    if (x1 >= s_ob->width) {
        x1 = s_ob->width - 1;
    }
    if (y1 >= s_ob->height) {
        y1 = s_ob->height - 1;
    }


    u8 rshift = s_ob->red_shift;
    u8 gshift = s_ob->green_shift;
    u8 bshift = s_ob->blue_shift;

    u8 *grayscale = bitmap->data.grayscale + (-cut_bot * bitmap->width) + (-cut_left);
    u32 *rgba = s_ob->pixels + y0 * s_ob->width + x0;
    for (i32 y = y0; y <= y1; y++) {
        u8 *alpha_row = grayscale;
        u32 *pixel_row = rgba;
        for (i32 x = x0; x <= x1; x++) {
            f32 alpha = *alpha_row++ / 255.0;

            u32 pixel = *pixel_row;
            f32 r0 = (pixel >> rshift) & 0xff;
            f32 g0 = (pixel >> gshift) & 0xff;
            f32 b0 = (pixel >> bshift) & 0xff;

            f32 r1 = r0 + (color.x - r0)*alpha;
            f32 g1 = g0 + (color.y - g0)*alpha;
            f32 b1 = b0 + (color.z - b0)*alpha;

            pixel = (u32)b1 << bshift | (u32)g1 << gshift | (u32)r1 << rshift;
            *pixel_row++ = pixel;
        }
        grayscale += bitmap->width;
        rgba += s_ob->width;
    }
}

void
draw_border(RectF32 rect, f32 size, V3F32 color)
{
    RectF32 draw_rect;

    // left
    draw_rect.x0 = rect.x0;
    draw_rect.y0 = rect.y0;
    draw_rect.x1 = rect.x0 + size;
    draw_rect.y1 = rect.y1;
    draw_rectf32(draw_rect, color);

    // bottom
    draw_rect.x0 = rect.x0;
    draw_rect.y0 = rect.y0;
    draw_rect.x1 = rect.x1;
    draw_rect.y1 = rect.y0 + size;
    draw_rectf32(draw_rect, color);

    // top
    draw_rect.x0 = rect.x0;
    draw_rect.y0 = rect.y1 - size;
    draw_rect.x1 = rect.x1;
    draw_rect.y1 = rect.y1;
    draw_rectf32(draw_rect, color);

    // right
    draw_rect.x0 = rect.x1 - size;
    draw_rect.y0 = rect.y0;
    draw_rect.x1 = rect.x1;
    draw_rect.y1 = rect.y1;
    draw_rectf32(draw_rect, color);
}

void
draw_cursor(V2F32 pos, Font *font, String32 *str, u32 cursor)
{
    f32 width = font->scale;
    f32 height = font->y_advance;
    f32 xoff = font_get_string32_cursor_xoff(font, str, cursor);
    V3F32 color = {0.0f, 0.0f, 0.0f};

    pos.x += xoff;

    RectF32 rect = {pos.x, pos.y, pos.x + width, pos.y + height};
    draw_rectf32(rect, color);
}

void
draw_string32(V2F32 pos, String32 *str, Font *font)
{
    V3F32 color = v3f32(0, 0, 0);

    pos.y += font->baseline;

    for (int i = 0; i < str->len; i++) {
        u32 c1 = str->codepoints[i];
        Glyph *glyph = font_get_glyph(font, c1);

        f32 draw_x = pos.x + glyph->xoff;
        f32 draw_y = pos.y + glyph->yoff;

        if (c1 != ' ') {
            Bitmap *mono_bitmap = &glyph->bitmap;
            V2F32 draw_pos = v2f32(draw_x, draw_y);
            draw_mono_bitmap(draw_pos, mono_bitmap, color);
        }

        u32 c2 = '\0';
        if (i < str->len -1) {
            c2 = str->codepoints[i+1];
        }
        f32 kern_advance = stbtt_GetCodepointKernAdvance(&font->info, c1, c2);
        f32 advance_width = glyph->advance_width + kern_advance;

        pos.x += advance_width;
    }
}

