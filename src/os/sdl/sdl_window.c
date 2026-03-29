#include <os/os.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL.h>
#include <GL/gl.h>


struct OSWindow {
    SDL_Window *sdl_window;
    SDL_GLContext gl_context;
    u32 gl_texture_id;
    OSOffscreenBuffer offscreen_buffer;
};


static void
os_offscreen_buffer_reinit(OSOffscreenBuffer *buffer, i32 w, i32 h)
{
    void *realloced_pixels = realloc(buffer->pixels, w*h*sizeof(buffer->pixels[0]));
    if (!realloced_pixels) {
        printf("could not resize offscreen buffer\n");
        return;
    }
    buffer->green_shift = 8;
    buffer->blue_shift = 16;
    buffer->red_shift = 0;
    buffer->alpha_shift = 24;
    buffer->width = w;
    buffer->height = h;
    buffer->pixels = realloced_pixels;
}

static void
os_offscreen_buffer_deinit(OSOffscreenBuffer *buffer)
{
    buffer->width = 0;
    buffer->height = 0;
    free(buffer->pixels);
}


OSWindow *
os_window_create(const char *name, i32 width, i32 height)
{
    OSWindow *window = malloc(sizeof(*window));
    memset(window, 0, sizeof(*window));

    SDL_Window *sdl_window = SDL_CreateWindow(name, width, height, SDL_WINDOW_OPENGL);
    if (!sdl_window) {
        printf("SDL_CreateWindow failed\n");
        return 0;
    }


    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);

    glGenTextures(1, &window->gl_texture_id);
    glBindTexture(GL_TEXTURE_2D, window->gl_texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


    window->sdl_window = sdl_window;
    window->gl_context = gl_context;
    os_offscreen_buffer_reinit(&window->offscreen_buffer, width, height);

    return window;
}

void
os_window_destroy(OSWindow *window)
{
    os_offscreen_buffer_deinit(&window->offscreen_buffer);
    SDL_GL_DestroyContext(window->gl_context);  
    SDL_DestroyWindow(window->sdl_window);
    free(window);
}

b32
os_window_get_event(OSWindow *window, OSEvent *event)
{
    SDL_Window *sdl_window = window->sdl_window;
    SDL_Event sdl_event;
    if (SDL_PollEvent(&sdl_event)) {
        if (sdl_event.type == SDL_EVENT_WINDOW_DESTROYED) {
            event->type = OS_EVENT_WINDOW_DESTROYED;
            return true;
        }
        else if (sdl_event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            SDL_DestroyWindow(sdl_window);
            event->type = OS_EVENT_WINDOW_DESTROYED;
            return true;
        }
        else if (sdl_event.type == SDL_EVENT_WINDOW_RESIZED) {
            event->type = OS_EVENT_WINDOW_RESIZE;
            event->ev.resize.width = sdl_event.window.data1;
            event->ev.resize.height = sdl_event.window.data2;
            os_offscreen_buffer_reinit(&window->offscreen_buffer, sdl_event.window.data1, sdl_event.window.data2);
            return true;
        }
        else if (sdl_event.type == SDL_EVENT_KEY_DOWN) {
            if ((sdl_event.key.key >= SDLK_SPACE && sdl_event.key.key <= SDLK_TILDE) ||
                sdl_event.key.key == '\t' ||
                sdl_event.key.key == '\r' ||
                sdl_event.key.key == '\b' ||
                sdl_event.key.key == SDLK_DELETE)
            {
                event->type = OS_EVENT_KEY_PRESS;
                event->ev.key_press.is_unicode = true;
                event->ev.key_press.code = sdl_event.key.key;
                return true;
            }
            else if (sdl_event.key.key == SDLK_LEFT) {
                event->type = OS_EVENT_KEY_PRESS;
                event->ev.key_press.is_unicode = false;
                event->ev.key_press.code = OS_KEYCODE_LEFT;
                return true;
            }
            else if (sdl_event.key.key == SDLK_RIGHT) {
                event->type = OS_EVENT_KEY_PRESS;
                event->ev.key_press.is_unicode = false;
                event->ev.key_press.code = OS_KEYCODE_RIGHT;
                return true;
            }
            else if (sdl_event.key.key == SDLK_UP) {
                event->type = OS_EVENT_KEY_PRESS;
                event->ev.key_press.is_unicode = false;
                event->ev.key_press.code = OS_KEYCODE_UP;
                return true;
            }
            else if (sdl_event.key.key == SDLK_DOWN) {
                event->type = OS_EVENT_KEY_PRESS;
                event->ev.key_press.is_unicode = false;
                event->ev.key_press.code = OS_KEYCODE_DOWN;
                return true;
            }
        }
    }

    return false;
}

OSOffscreenBuffer*
os_window_get_offscreen_buffer(OSWindow *window)
{
    return &window->offscreen_buffer;
}

void os_window_swap_buffers(OSWindow *window, OSOffscreenBuffer *offscreen_buffer)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, window->gl_texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, offscreen_buffer->width, offscreen_buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, offscreen_buffer->pixels);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f, -1.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f,  1.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    SDL_GL_SwapWindow(window->sdl_window);
}

