#include "basic/basic.h"
#include <glad/gl.h>
#include <os/os.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL.h>
#include <GL/gl.h>


struct OSWindow {
    SDL_Window *sdl_window;
    SDL_GLContext gl_context;
    i32 width;
    i32 height;
};


OSWindow *
os_window_create(const char *name, i32 width, i32 height)
{
    OSWindow *window = malloc(sizeof(*window));
    memset(window, 0, sizeof(*window));

    u32 gl_major = 3;
    u32 gl_minor = 3;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, gl_major);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, gl_minor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *sdl_window = SDL_CreateWindow(name, width, height, SDL_WINDOW_OPENGL);
    if (!sdl_window) {
        printf("SDL_CreateWindow failed\n");
        return 0;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(sdl_window);
    int version = gladLoadGL((GLADloadfunc) SDL_GL_GetProcAddress);
    if (GLAD_VERSION_MAJOR(version) < gl_major ||
        (GLAD_VERSION_MAJOR(version) == gl_major && GLAD_VERSION_MINOR(version) < gl_minor))
    {
        printf("GL %d.%d is incompatible\n",
               GLAD_VERSION_MAJOR(version),
               GLAD_VERSION_MINOR(version));
    }


    window->sdl_window = sdl_window;
    window->gl_context = gl_context;
    window->width = width;
    window->height = height;

    return window;
}

void
os_window_destroy(OSWindow *window)
{
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
            window->width = sdl_event.window.data1;
            window->height = sdl_event.window.data2;
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

void
os_window_swap_buffers(OSWindow *window)
{
    SDL_GL_SwapWindow(window->sdl_window);
}

i32
os_window_get_w(OSWindow *window)
{
    return window->width;
}

i32
os_window_get_h(OSWindow *window)
{
    return window->height;
}

