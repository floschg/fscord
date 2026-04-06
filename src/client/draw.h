#include <os/os.h>
#include <client/font.h>
#include <basic/math.h>
#include <basic/string32.h>


void init_drawing(OSOffscreenBuffer *screen);

void draw_rectf32(RectF32 rect, V3F32 color);
void draw_mono_bitmap(V2F32 pos, Bitmap *bitmap, V3F32 color);
void draw_string32(V2F32 pos, String32 *text, Font *font);
void draw_cursor(V2F32 pos, Font *font, String32 *str, u32 cursor);
void draw_border(RectF32 rect, f32 size, V3F32 color);
