#ifndef FSCORD_H
#define FSCORD_H

#include <os/os.h>
#include <basic/time.h>
#include <client/sound.h>
#include <client/font.h>
#include <client/login.h>
#include <client/session.h>
#include <client/server_connection.h>

typedef struct Fscord {
    Arena perma_arena;
    Arena frame_arena;

    OSWindow *window;
    OSOffscreenBuffer *offscreen_buffer;
    OSSoundPlayer *sound_player;
    OSSoundBuffer *sound_buffer;

    Font font;

    Sound *sound_user_connected;
    Sound *sound_user_disconnected;
    PlaySound ps_user_connected;
    PlaySound ps_user_disconnected;

    b32 is_logged_in;
    Login login;
    Session session;
} Fscord;

extern Fscord g_fscord;

#endif // FSCORD_H
