#include <client/fscord.h>
#include <basic/basic.h>
#include <basic/arena.h>
#include <basic/string32.h>
#include <os/os.h>
#include <messages/messages.h>
#include <client/login.h>
#include <client/session.h>
#include <client/string32_handles.h>
#include <client/server_connection.h>

#include <string.h>


Fscord g_fscord;


static b32
fscord_update(Fscord *fscord)
{
    arena_clear(&fscord->frame_arena);


    Login *login = fscord->login;
    Session *session = &fscord->session;


    // handle network events
    // Todo: Do this more carefully.
    ServerConnectionStatus status = server_connection_get_status();
    if (status == SERVER_CONNECTION_NOT_ESTABLISHED) {
        if (fscord->is_logged_in) {
            fscord->is_logged_in = false;
            session_reset(session);
        }
        if (login->is_trying_to_login) {
            login->warning = SH_LOGIN_WARNING_COULD_NOT_CONNECT;
            login->is_trying_to_login = false;
        }
    }
    else if (status == SERVER_CONNECTION_ESTABLISHED) {
        if (!fscord->is_logged_in && login->is_trying_to_login) {
            login_update_login_attempt(login);
        }

        b32 handled = server_connection_handle_events();
        if (!handled) {
            fscord->is_logged_in = false;
            session_reset(session);
            server_connection_terminate();
            login->warning = SH_LOGIN_WARNING_CONNECTION_LOST;
        }
    }
    else if (status == SERVER_CONNECTION_ESTABLISHING) {
        // do nothing
    }
    else {
        InvalidCodePath;
    }


    // handle window events
    OSEvent event;
    while (os_window_get_event(fscord->window, &event)) {
        if (event.type == OS_EVENT_WINDOW_RESIZE) {
            continue;
        }
        if (event.type == OS_EVENT_WINDOW_DESTROYED) {
            return false;
        }

        if (fscord->is_logged_in) {
            session_process_event(session, &event);
        }
        else {
            login_process_event(login, &event);
        }
    }

    if (fscord->is_logged_in) {
        session_draw(session);
    }
    else {
        login_draw(login);
    }

#if 0
    OSSoundBuffer *sound_buffer = os_sound_player_get_buffer(fscord->sound_player);
    play_sound_update(&fscord->ps_user_connected, sound_buffer);
    play_sound_update(&fscord->ps_user_disconnected, sound_buffer);
#endif


    return true;
}


static b32
fscord_init(Fscord *fscord)
{
    memset(fscord, 0, sizeof(*fscord));


    arena_init(&fscord->perma_arena, MEBIBYTES(10));
    arena_init(&fscord->frame_arena, MEBIBYTES(10));


    string32_handles_load_language();


    fscord->window = os_window_create("fscord", 1024, 720);
    if (!fscord->window) {
        return false;
    }
    fscord->offscreen_buffer = os_window_get_offscreen_buffer(fscord->window);


#if 0
    fscord->sound_player = os_sound_player_create(&fscord->perma_arena, 44100);
    if (!fscord->sound_player) {
        return false;
    }
#endif


    if (!font_init(&fscord->font, "./Inconsolata-Regular.ttf", 20)) {
        return false;
    }
    fscord->sound_user_connected = sound_load(SOUND_USER_CONNECTED);
    fscord->sound_user_disconnected = sound_load(SOUND_USER_DISCONNECTED);

    os_net_secure_streams_init(&fscord->perma_arena, 1);
    server_connection_create(&fscord->perma_arena);

    fscord->is_logged_in = false;
    fscord->login = login_create(&fscord->perma_arena, fscord);
    session_init(&fscord->session);

    return true;
}


static void
fscord_main(void)
{
    if (!fscord_init(&g_fscord)) {
        return;
    }

    b32 running = true;
    while (running) {
        running = fscord_update(&g_fscord);

        os_window_swap_buffers(g_fscord.window, g_fscord.offscreen_buffer);
        //os_sound_player_play(fscord->sound_player);
    }
}


int
main(void)
{
    fscord_main();
    return 0;
}

