#include "messages/messages.h"
#include <server/s2c_sender.h>

static Arena s_send_arena;

void
send_s2c_chat_message(Client *client, String32 *username, String32 *content, OSTime now)
{
    OSTime time = os_time_get_now();
    Arena *arena = &s_send_arena;


    S2C_ChatMessage *chat_message = arena_push(arena, sizeof(S2C_ChatMessage));

    String32 *username_copy = string32_create_from_string32(arena, username);
    chat_message->username.offset = ((u8*)username_copy - (u8*)chat_message);

    String32 *content_copy = string32_create_from_string32(arena, content);
    chat_message->content.offset = ((u8*)content_copy - (u8*)chat_message);

    chat_message->epoch_time_seconds = time.seconds;
    chat_message->epoch_time_nanoseconds = time.nanoseconds;

    chat_message->header.type = S2C_CHAT_MESSAGE;
    chat_message->header.size = arena->size_used;


    os_net_secure_stream_send(client->sstream_id, arena->memory, arena->size_used);
    arena_clear(arena);
}


void
send_s2c_user_update(Client *client, String32 *username, u32 online_status)
{
    Arena *arena = &s_send_arena;

    S2C_UserUpdate *user_update = arena_push(arena, sizeof(S2C_UserUpdate));
    String32 *username_copy = string32_create_from_string32(arena, username);

    user_update->status = online_status;
    user_update->username.offset = (u8*)username_copy - (u8*)user_update;

    user_update->header.type = S2C_USER_UPDATE;
    user_update->header.size = arena->size_used;


    os_net_secure_stream_send(client->sstream_id, arena->memory, arena->size_used);
    arena_clear(arena);
}


void
send_s2c_login(Client *client, u32 login_result)
{
    Arena *arena = &s_send_arena;


    S2C_Login *login = arena_push(arena, sizeof(S2C_Login));
    login->login_result = login_result;

    login->header.type = S2C_LOGIN;
    login->header.size = arena->size_used;


    os_net_secure_stream_send(client->sstream_id, arena->memory, arena->size_used);
    arena_clear(arena);
}


void
s2c_sender_init(Arena *arena)
{
    arena_init(&s_send_arena, 1408);
}

