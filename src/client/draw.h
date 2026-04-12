#include <os/os.h>
#include <client/font.h>
#include <basic/math.h>
#include <basic/string32.h>

bool r_init(OSWindow *window);
void r_begin(void);
void r_end(void);

void draw_aabb(AABB aabb, V3F32 color);
void draw_mono_bitmap(V3F32 pos, Bitmap *bitmap, V3F32 color);
void draw_string32(V3F32 pos, String32 *text, Font *font);
void draw_cursor(V3F32 pos, Font *font, String32 *str, u32 cursor);
void draw_border(AABB aabb, f32 size, V3F32 color);

i32 r_get_window_w(void);
i32 r_get_window_h(void);

