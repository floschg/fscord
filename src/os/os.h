#ifndef OS_H
#define OS_H

#include <basic/arena.h>
#include <basic/basic.h>
#include <openssl/rsa.h>


void* os_memory_alloc(size_t size);
void  os_memory_free(void *mem);


typedef struct {
    i64 seconds;
    i64 nanoseconds;
} OSTime;

OSTime os_time_get_now(void);


typedef void OSLibrary;
OSLibrary* os_library_open(const char *path);
void       os_library_close(OSLibrary *lib);
void*      os_library_get_proc(OSLibrary *lib, const char *name);


char*   os_file_read_as_string(Arena *arena, char *path, size_t *outlen);


typedef enum {
    OS_EVENT_KEY_PRESS,
    OS_EVENT_KEY_RELEASE,
    OS_EVENT_WINDOW_RESIZE,
    OS_EVENT_WINDOW_DESTROYED,
} OSEventType;

typedef enum {
    OS_KEYCODE_LEFT,
    OS_KEYCODE_RIGHT,
    OS_KEYCODE_UP,
    OS_KEYCODE_DOWN,
} OSEventKeySpecial;

typedef struct {
    b32 is_unicode; // else it's "special", see OSKeySpecial
    u32 code;   // unicode character or some other keyboard keys
} OSEventKeyPress;

typedef struct {
    b32 is_special;
    u32 code;   // unicode character or some other keyboard keys
} OSEventKeyRelease;

typedef struct {
    i32 width;
    i32 height;
} OSEventResize;

typedef struct OSEvent {
    OSEventType type;
    union {
        OSEventKeyPress key_press;
        OSEventKeyRelease key_release;
        OSEventResize resize;
    } ev;
} OSEvent;


// Todo: switch to gpu-rendering (probably opengl)
typedef struct {
    u8 red_shift;
    u8 green_shift;
    u8 blue_shift;
    u8 alpha_shift;
    i32 width;
    i32 height;
    u32 *pixels;
} OSOffscreenBuffer;

typedef struct OSWindow OSWindow;
OSWindow*          os_window_create(const char *name, i32 width, i32 height);
void               os_window_destroy(OSWindow *window);
b32                os_window_get_event(OSWindow *window, OSEvent *event);
OSOffscreenBuffer* os_window_get_offscreen_buffer(OSWindow *window);
void               os_window_swap_buffers(OSWindow *window, OSOffscreenBuffer *offscreen_buffer);



typedef enum {
    OS_NET_SECURE_STREAM_ERROR,
    OS_NET_SECURE_STREAM_DISCONNECTED,

    OS_NET_SECURE_STREAM_CONNECTED,
    OS_NET_SECURE_STREAM_HANDSHAKING,
} OSNetSecureStreamStatus;

typedef u32 OSNetSecureStreamId;
#define OS_NET_SECURE_STREAM_ID_INVALID U32_MAX

void os_net_secure_streams_init(Arena *arena, size_t max_count);

OSNetSecureStreamStatus os_net_secure_stream_get_status(OSNetSecureStreamId id);
i64  os_net_secure_stream_error(OSNetSecureStreamId id);

OSNetSecureStreamId os_net_secure_stream_listen(u16 port, EVP_PKEY *server_rsa_pri);
OSNetSecureStreamId os_net_secure_stream_accept(OSNetSecureStreamId listener);
OSNetSecureStreamId os_net_secure_stream_connect(char *address, u16 port, EVP_PKEY *server_rsa_pub);
void os_net_secure_stream_close(OSNetSecureStreamId id);

i64  os_net_secure_stream_send(OSNetSecureStreamId id, u8 *buffer, size_t size);
i64  os_net_secure_stream_recv(OSNetSecureStreamId id, u8 *buffer, size_t size);
int  os_net_secure_stream_get_fd(OSNetSecureStreamId id);



typedef struct OSSoundPlayer OSSoundPlayer;

typedef struct {
    i32 play_cursor;
    i32 max_sample_count;
    i32 sample_count;
    i32 samples_per_second;
    i16 *samples;
} OSSoundBuffer;

// Todo: maybe change api by replacing get_buffer with play_buffer
OSSoundPlayer* os_sound_player_create(Arena *arena, i32 samples_per_second);
OSSoundBuffer* os_sound_player_get_buffer(OSSoundPlayer *player);
void           os_sound_player_close(OSSoundPlayer *player);



// Note: api unused and in progress

typedef struct {
    void (*fn_worker)(void *data);
    void *data;
} OSWork;

typedef struct OSThreadPool OSThreadPool;
typedef u32 OSThreadId;

OSThreadPool* os_thread_pool_create(Arena *arena, u32 thread_count, u32 work_queue_size);
OSThreadId  os_thread_pool_start(OSThreadPool *pool, OSWork work);
void        os_thread_pool_finish(OSThreadPool *pool, OSThreadId id);



#endif // OS_H

