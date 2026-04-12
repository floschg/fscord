#include "basic/basic.h"
#include "cglm/cam.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include <glad/gl.h>
#include <GL/gl.h>
#include <os/os.h>
#include <basic/math.h>
#include <client/draw.h>
#include <client/fscord.h>

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <string.h>


typedef struct {
    u32 vbo;
    u32 cur_len;
    u32 max_len;
    float *data;
} VertexBuffer;

typedef struct {
    u32 ebo;
    u32 cur_val;
    u32 cur_len;
    u32 max_len;
    u32 *data;
} IndexBuffer;

typedef struct {
    V3F32 pos;
    Bitmap *bitmap;
    V3F32 color;
} MonoBitmapEntry;


static OSWindow *s_window;
static i32 s_window_w;
static i32 s_window_h;

static u32 s_basic_vao;
static u32 s_basic_program;
static VertexBuffer s_basic_vb;
static IndexBuffer s_basic_ib;

static u32 s_text_vao;
static u32 s_text_program;
static VertexBuffer s_text_vb;
static IndexBuffer s_text_ib;

static Arena s_mono_bitmaps;


static char *s_basic_vshader_src = {
    "#version 330 core\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec3 a_col;\n"
    "\n"
    "uniform mat4 ortho;\n"
    "\n"
    "out vec4 v_col;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = ortho * vec4(a_pos, 1.0);\n"
    "   v_col = vec4(a_col, 1.0);\n"
    "\n"
    "}\0"
};

static char *s_basic_fshader_src = {
    "#version 330 core\n"
    "\n"
    "in vec4 v_col;\n"
    "\n"
    "out vec4 frag_col;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    frag_col = v_col;\n"
    "}\0"
};

static char *s_text_vshader_src = {
    "#version 330 core\n"
    "\n"
    "layout (location = 0) in vec3 a_pos;\n"
    "layout (location = 1) in vec3 a_col;\n"
    "layout (location = 2) in vec2 a_tex_coord;\n"
    "\n"
    "out vec3 v_col;\n"
    "out vec2 v_tex_coord;\n"
    "\n"
    "uniform mat4 ortho;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = ortho * vec4(a_pos, 1.0);\n"
    "   v_col = a_col;\n"
    "   v_tex_coord = a_tex_coord;\n"
    "}\0"
};

static char *s_text_fshader_src = {
    "#version 330 core\n"
    "\n"
    "in vec3 v_col;\n"
    "in vec2 v_tex_coord;\n"
    "\n"
    "out vec4 frag_col;\n"
    "\n"
    "uniform sampler2D texture0;"
    "\n"
    "void main()\n"
    "{\n"
    "    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture0, v_tex_coord).r);\n"
    "    frag_col = vec4(v_col, 1.0) * sampled;\n"
    "}\0"
};


i32
r_get_window_w(void)
{
    return s_window_w;
}

i32
r_get_window_h(void)
{
    return s_window_h;
}



// Todo: vertices and indices might get full at different times. that's not good.
static void
push_vertices(VertexBuffer *vb, float *vertices, u32 len)
{
    if (len + vb->cur_len <= vb->max_len) {
        float *dest = vb->data + vb->cur_len;
        memcpy(dest, vertices, len * sizeof(vertices[0]));
        vb->cur_len += len;
    }
    else {
        printf("CRITICAL ERROR: OUT OF MEMORY FOR push_vertices\n");
        exit(0);
    }
}

// Todo: vertices and indices might get full at different times. that's not good.
static void
push_indices(IndexBuffer *ib, u32 *indices, u32 len)
{
    if (len + ib->cur_len <= ib->max_len) {
        u32 *dest = ib->data + ib->cur_len;
        memcpy(dest, indices, len * sizeof(indices[0]));
        ib->cur_len += len;
    }
    else {
        printf("CRITICAL ERROR: OUT OF MEMORY FOR push_indices\n");
        exit(0);
    }
}


static void
destroy_shader(u32 shader)
{
    glDeleteShader(shader);
}

static u32
create_and_compile_shader(u32 type, const char *src)
{
    assert(type == GL_VERTEX_SHADER || type == GL_FRAGMENT_SHADER);

    u32 shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        if (type == GL_VERTEX_SHADER) {
            printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", info_log);
        }
        else if (type == GL_FRAGMENT_SHADER) {
            printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", info_log);
        }
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static u32
create_program(const char *vshader_src, const char *fshader_src)
{
    u32 vshader = create_and_compile_shader(GL_VERTEX_SHADER, vshader_src);
    u32 fshader = create_and_compile_shader(GL_FRAGMENT_SHADER, fshader_src);
    if (!vshader || !fshader) {
        return 0;
    }

    u32 program = glCreateProgram();
    if (!program) {
        printf("ERROR::PROGRAM::CREATE_FAILED\n");
        return 0;
    }
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);
    glLinkProgram(program);

    GLint program_linked;
    glGetProgramiv(program, GL_LINK_STATUS, &program_linked);
    if (program_linked != GL_TRUE) {
        GLsizei log_length = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &log_length, message);
        printf("ERROR::PROGRAM::LINK_FAILED\n%s\n", message);

        glDeleteProgram(program);
        program = 0;
    }

    destroy_shader(vshader);
    destroy_shader(fshader);
    return program;
}

void
draw_aabb(AABB aabb, V3F32 color)
{
    u32 i0 = s_basic_ib.cur_val;
    float vertices[] = {
        aabb.x0, aabb.y0, aabb.z, color.x, color.y, color.z, // bot left
        aabb.x1, aabb.y0, aabb.z, color.x, color.y, color.z, // bot right
        aabb.x1, aabb.y1, aabb.z, color.x, color.y, color.z, // top right
        aabb.x0, aabb.y1, aabb.z, color.x, color.y, color.z  // top left
    };
    u32 indices[] = {
        i0+0, i0+1, i0+2,
        i0+0, i0+2, i0+3
    };
    push_vertices(&s_basic_vb, vertices, ARRAY_COUNT(vertices));
    push_indices(&s_basic_ib, indices, ARRAY_COUNT(indices));
    s_basic_ib.cur_val += 4;
}

void
draw_border(AABB aabb, f32 size, V3F32 color)
{
    float left_x1  = aabb.x0 + size;
    float right_x0 = aabb.x1 - size;
    float top_y0   = aabb.y1 - size;
    float bot_y1   = aabb.y0 + size;
    float vertices[] = {
        aabb.x0, aabb.y0, aabb.z, color.x, color.y, color.z, // 0 bot-border bot-left
        aabb.x1, aabb.y0, aabb.z, color.x, color.y, color.z, // 1 bot-border bot-right
        aabb.x1, bot_y1,  aabb.z, color.x, color.y, color.z, // 2 bot-border top-right
        aabb.x0, bot_y1,  aabb.z, color.x, color.y, color.z, // 3 bot-border top-left

        left_x1, aabb.y0, aabb.z, color.x, color.y, color.z, // 4 left-border bot-right
        left_x1, aabb.y1, aabb.z, color.x, color.y, color.z, // 5 left-border top-right
        aabb.x0, aabb.y1, aabb.z, color.x, color.y, color.z, // 6 left-border top-left

        right_x0, bot_y1, aabb.z, color.x, color.y, color.z, // 7 right-border bot-left
        aabb.x1, aabb.y1, aabb.z, color.x, color.y, color.z, // 8 right-border top-right
        right_x0,aabb.y1, aabb.z, color.x, color.y, color.z, // 9 right-border top-left

        left_x1,  top_y0, aabb.z, color.x, color.y, color.z, // 10 top-border bot-left
        right_x0, top_y0, aabb.z, color.x, color.y, color.z, // 11 top-border bot-right
    };
    u32 i0 = s_basic_ib.cur_val;
    u32 indices[] = {
        i0+0,  i0+1,  i0+2, i0+0,  i0+2, i0+3,  // bot-border
        i0+3,  i0+4,  i0+5, i0+3,  i0+5, i0+6,  // left-border
        i0+7,  i0+2,  i0+8, i0+7,  i0+8, i0+9,  // right-border
        i0+10, i0+11, i0+9, i0+10, i0+9, i0+5   // top-border
    };
    push_vertices(&s_basic_vb, vertices, ARRAY_COUNT(vertices));
    push_indices(&s_basic_ib, indices, ARRAY_COUNT(indices));
    s_basic_ib.cur_val += 12;
}

void
draw_mono_bitmap(V3F32 pos, Bitmap *bitmap, V3F32 color)
{
    MonoBitmapEntry *entry = arena_push(&s_mono_bitmaps, sizeof(MonoBitmapEntry));
    entry->pos = pos;
    entry->bitmap = bitmap;
    entry->color = color;
}

void
draw_cursor(V3F32 pos, Font *font, String32 *str, u32 cursor)
{
    f32 width = font->scale * 100;
    f32 height = font->y_advance;
    f32 xoff = font_get_string32_cursor_xoff(font, str, cursor);
    V3F32 color = {0.0f, 0.0f, 0.0f};

    pos.x += xoff;

    AABB aabb = {pos.x, pos.y, pos.x + width, pos.y + height, pos.z};
    draw_aabb(aabb, color);
}

void
draw_string32(V3F32 pos, String32 *str, Font *font)
{
    V3F32 color = {0.0f, 0.0f, 0.0f};

    pos.y += font->baseline;
    for (int i = 0; i < str->len; i++) {
        u32 c1 = str->codepoints[i];
        Glyph *glyph = font_get_glyph(font, c1);

        f32 draw_x = pos.x + glyph->xoff;
        f32 draw_y = pos.y + glyph->yoff;

        if (c1 != ' ') {
            Bitmap *mono_bitmap = &glyph->bitmap;
            V3F32 draw_pos = {draw_x, draw_y, pos.z};
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

static void
do_text_drawing(void)
{
    glBindVertexArray(s_text_vao);


    glUseProgram(s_text_program);
    mat4 ortho;
    glm_ortho(0.0f, s_window_w, 0.0f, s_window_h, -1.0f, 1.0f, ortho);
    GLuint ortho_loc = glGetUniformLocation(s_text_program, "ortho");
    glUniformMatrix4fv(ortho_loc, 1, GL_FALSE, (f32*)ortho);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   


    u32 texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);  

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glUniform1i(glGetUniformLocation(s_text_program, "texture0"), 0);


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  


    u64 count = s_mono_bitmaps.size_used / sizeof(MonoBitmapEntry);
    MonoBitmapEntry *entry = (MonoBitmapEntry*)s_mono_bitmaps.data;
    while (count--) {
        V3F32 pos = entry->pos;
        V3F32 col = entry->color;
        Bitmap *bitmap = entry->bitmap;

        float x0 = entry->pos.x;
        float y0 = entry->pos.y;
        float x1 = entry->pos.x + entry->bitmap->width;
        float y1 = entry->pos.y + entry->bitmap->height;
        float vertices[] = {
            x0, y0, pos.z, col.x, col.y, col.z, 0.0f, 0.0f, // bot-left
            x1, y0, pos.z, col.x, col.y, col.z, 1.0f, 0.0f, // bot-right
            x1, y1, pos.z, col.x, col.y, col.z, 1.0f, 1.0f, // top-right
            x0, y1, pos.z, col.x, col.y, col.z, 0.0f, 1.0f  // top-left
        };
        u32 indices[] = {
            0, 1, 2,
            0, 2, 3
        };
        push_vertices(&s_text_vb, vertices, ARRAY_COUNT(vertices));
        push_indices(&s_text_ib, indices, ARRAY_COUNT(indices));


        glBindBuffer(GL_ARRAY_BUFFER, s_text_vb.vbo);
        u32 vertices_size = s_text_vb.cur_len * sizeof(f32);
        float *vertices_data = s_text_vb.data;
        glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);


        u32 indices_size = s_text_ib.cur_len * sizeof(u32);
        u32 *indices_data = s_text_ib.data;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);


        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, bitmap->width, bitmap->height, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap->data.alpha);


        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


        s_text_vb.cur_len = 0;
        s_text_ib.cur_len = 0;

        entry++;
    }

    glDisable(GL_BLEND);
    glDeleteTextures(1, &texture);
}

static bool
init_text_drawing(void)
{
    Arena *arena = &g_fscord.perma_arena;
    VertexBuffer *vb = &s_text_vb;
    IndexBuffer *ib = &s_text_ib;

    // setup vao
    glGenVertexArrays(1, &s_text_vao);
    glBindVertexArray(s_text_vao);

    // setup vbo
    glGenBuffers(1, &vb->vbo);  
    glBindBuffer(GL_ARRAY_BUFFER, vb->vbo);
    vb->max_len = MEBIBYTES(1);
    vb->data = arena_push(arena, vb->max_len* sizeof(f32));

    // setup ebo
    glGenBuffers(1, &ib->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->ebo);
    ib->max_len = MEBIBYTES(1);
    ib->data = arena_push(arena, ib->max_len * sizeof(u32));

    // setup shader
    s_text_program = create_program(s_text_vshader_src, s_text_fshader_src);
    if (!s_text_program) {
        return false;
    }
    // - positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(0 * sizeof(float)));
    glEnableVertexAttribArray(0);  
    // - colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);  
    // - texture coordinates
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);  


    return true;
}

static void
do_basic_drawing(void)
{
    glBindVertexArray(s_basic_vao);

    glBindBuffer(GL_ARRAY_BUFFER, s_basic_vb.vbo);
    u32 vertices_size = s_basic_vb.cur_len * sizeof(f32);
    float *vertices_data = s_basic_vb.data;
    glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices_data, GL_STATIC_DRAW);

    u32 indices_size = s_basic_ib.cur_len * sizeof(u32);
    u32 *indices_data = s_basic_ib.data;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices_data, GL_STATIC_DRAW);


    glUseProgram(s_basic_program);
    mat4 ortho;
    glm_ortho(0.0f, s_window_w, 0.0f, s_window_h, -1.0f, 1.0f, ortho);
    GLuint ortho_loc = glGetUniformLocation(s_basic_program, "ortho");
    glUniformMatrix4fv(ortho_loc, 1, GL_FALSE, (f32*)ortho);


    glDrawElements(GL_TRIANGLES, s_basic_ib.cur_len, GL_UNSIGNED_INT, 0);
}

static bool
init_basic_drawing(void)
{
    Arena *arena = &g_fscord.perma_arena;
    VertexBuffer *vb = &s_basic_vb;
    IndexBuffer *ib = &s_basic_ib;


    // setup vao
    glGenVertexArrays(1, &s_basic_vao);
    glBindVertexArray(s_basic_vao);

    // setup vbo
    glGenBuffers(1, &vb->vbo);  
    glBindBuffer(GL_ARRAY_BUFFER, vb->vbo);
    vb->max_len = MEBIBYTES(1);
    vb->data = arena_push(arena, vb->max_len* sizeof(f32));

    // setup ebo
    glGenBuffers(1, &ib->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->ebo);
    ib->max_len = MEBIBYTES(1);
    ib->data = arena_push(arena, ib->max_len * sizeof(u32));


    // setup shader
    s_basic_program = create_program(s_basic_vshader_src, s_basic_fshader_src);
    if (!s_basic_program) {
        return false;
    }
    // - shader positions
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);  
    // - shader colors
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);


    return true;
}

void
r_end(void)
{
    do_basic_drawing();
    do_text_drawing();
}

void
r_begin(void)
{
    i32 window_w = os_window_get_w(s_window);
    i32 window_h = os_window_get_h(s_window);
    if(window_w != s_window_w || window_h != s_window_h) {
        s_window_w = window_w;
        s_window_h = window_h;
        glViewport(0, 0, window_w, window_h);
    }

    s_basic_vb.cur_len = 0;
    s_basic_ib.cur_len = 0;
    s_basic_ib.cur_val = 0;

    s_text_vb.cur_len = 0;
    s_text_ib.cur_len = 0;
    s_text_ib.cur_val = 0;

    s_mono_bitmaps.size_used = 0;

    glClearColor(0.2f, 0.2f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

bool
r_init(OSWindow *window)
{
    s_window = window;

    s_mono_bitmaps = arena_make_subarena(&g_fscord.perma_arena, MEBIBYTES(1));

    if (!init_basic_drawing()) {
        return false;
    }
    if (!init_text_drawing()) {
        return false;
    }
    glEnable(GL_DEPTH_TEST);

    return true;
}

