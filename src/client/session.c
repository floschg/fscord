#include "basic/string32.h"
#include "os/os.h"
#include "client/fscord.h"
#include "client/session.h"
#include "client/draw.h"
#include "client/font.h"
#include "messages/messages.h"

#include <string.h>

extern Fscord fscord;


static void
chat_message_init(ChatMessage *chatmsg, Arena *arena)
{
    memset(chatmsg, 0, sizeof(*chatmsg));
    chatmsg->sender_name = string32_buffer_create(arena, MESSAGES_MAX_USERNAME_LEN);
    chatmsg->content = string32_buffer_create(arena, MESSAGES_MAX_MESSAGE_LEN);
}

static void
session_draw_chat_message(ChatMessage *message, V3F32 pos)
{
    Font *font = &g_fscord.font;
    Arena *frame_arena = &g_fscord.frame_arena;

    f32 str_width;
    f32 dx = font->y_advance / 2.f;


    pos.x += font->y_advance;


    // draw local time
    time_t local_time_src = message->creation_time.seconds;
	struct tm *local_time = localtime(&local_time_src);

    char time_cstr_len = 5;
    char time_cstr[time_cstr_len+1];
    sprintf(time_cstr, "%.2d:%.2d", local_time->tm_hour, local_time->tm_min);

    String32 *time_str = string32_create_from_ascii(frame_arena, time_cstr);
    draw_string32(pos, time_str, font);
    str_width = font_get_string32_width(font, time_str);
    pos.x += str_width + dx;


    // draw sender_name
    String32Buffer *sender_name_buff = string32_buffer_create(frame_arena, message->sender_name->len + 2);
    string32_buffer_append_ascii_cstr(sender_name_buff, "<");
    string32_buffer_append_string32_buffer(sender_name_buff, message->sender_name);
    string32_buffer_append_ascii_cstr(sender_name_buff, ">");

    String32 *sender_name = string32_buffer_to_string32(frame_arena, sender_name_buff);
    draw_string32(pos, sender_name, font);
    str_width = font_get_string32_width(font, sender_name);
    pos.x += str_width + dx;


    // draw content
    String32 *content = string32_buffer_to_string32(frame_arena, message->content);
    draw_string32(pos, content, font);
}

static void
session_draw_chat(Session *session, AABB aabb)
{
    Font *font = &g_fscord.font;

    float z_border = aabb.z + 0.02f;
    float z_chat_message = aabb.z + 0.01f;


    // draw background
    V3F32 bg_color = {0.2f, 0.2f, 0.3f};
    draw_aabb(aabb, bg_color);


    // draw border
    aabb.z = z_border;
    f32 border_size = font->y_advance / 8.f;
    V3F32 border_color = {0, 0, 0};
    draw_border(aabb, border_size, border_color);


    // draw messages
    V3F32 pos = {
        aabb.x0 + border_size*4,
        aabb.y0 + border_size*4,
        z_chat_message
    };
    f32 dy = font->y_advance + border_size * 4;

    RingAllocIt it;
    ring_alloc_it_init(&it, &session->chat_message_alloc);

    ChatMessage *chat_msg = ring_alloc_it_next(&it);
    while (chat_msg) {
        if (pos.y >= aabb.y1) {
            break;
        }
        session_draw_chat_message(chat_msg, pos);
        pos.y += dy;

        chat_msg = ring_alloc_it_next(&it);
    }
}

static void
session_draw_prompt(Session *session, AABB aabb)
{
    Arena *frame_arena = &g_fscord.frame_arena;
    Font *font = &g_fscord.font;

    float z_text = aabb.z + 0.01f;
    float z_border = aabb.z + 0.02f;


    // draw background
    V3F32 bg_color = {0.2f, 0.3f, 0.2f};
    draw_aabb(aabb, bg_color);


    // draw border
    aabb.z = z_border;
    f32 border_size = font->y_advance / 8.f;
    V3F32 border_color = {0.16f, 0.32f, 0.08f};
    draw_border(aabb, border_size, border_color);


    // draw text
    f32 xmargin = border_size * 4;
    f32 ymargin = border_size * 4;
    V3F32 pos = {
        aabb.x0 + xmargin,
        aabb.y0 + ymargin,
        z_text
    };
    String32 *prompt_str = string32_buffer_to_string32(frame_arena, session->prompt);
    draw_string32(pos, prompt_str, font);
    draw_cursor(pos, font, prompt_str, session->prompt->cursor);
}

static void
session_draw_users(Session *session, AABB aabb_users)
{
    Font *font = &g_fscord.font;
    Arena *frame_arena = &g_fscord.frame_arena;
    AABB aabb = aabb_users;

    float z_usernames = aabb_users.z + 0.01f;
    float z_border = aabb_users.z + 0.02f;


    // draw background
    V3F32 bg_color = {0.3f, 0.2f, 0.2f};
    draw_aabb(aabb, bg_color);


    // draw border
    aabb.z = z_border;
    f32 border_size = font->y_advance / 8.f;
    V3F32 border_color = {0.0f, 0.0f, 0.0f};
    draw_border(aabb, border_size, border_color);


    // draw users
    f32 xmargin = border_size * 2;
    f32 ymargin = border_size * 2;
    f32 dy = font->y_advance;
    V3F32 pos = {
        aabb.x0 + xmargin,
        aabb.y1 - ymargin - dy,
        z_usernames
    };
    for (size_t i = 0; i < session->cur_user_count; i++) {
        if (pos.y < ymargin - font->y_advance) {
            break;
        }
        User *user = &session->users[i];

        // Todo: we want to cut characters on rectangle boundaries
        size_t name_len_desired = user->name->len;
        size_t name_len_avail = font->scale * 1000; // Todo: find better value
        size_t name_len = name_len_desired <= name_len_avail ? name_len_desired : name_len_avail;
        String32 *username = string32_buffer_to_string32_with_len(frame_arena, user->name, name_len);
        draw_string32(pos, username, font);
        pos.y -= dy;
    }
}

void
session_draw(Session *session)
{
    float window_w = r_get_window_w();
    float window_h = r_get_window_h();

    f32 left_width = window_w * 0.3f;
    f32 prompt_height = window_h * 0.1f;

    AABB aabb_users = {0.0f, 0.0f, left_width, window_h, 0.0f};
    AABB aabb_prompt = {left_width, 0.0f, window_w, prompt_height, 0.0f};
    AABB aabb_chat = {left_width, prompt_height, window_w, window_h, 0.0f};

    session_draw_users(session, aabb_users);
    session_draw_prompt(session, aabb_prompt);
    session_draw_chat(session, aabb_chat);
}

void
session_add_chat_message(Session *session, Time creation_time, String32 *sender_name, String32 *content)
{
    ChatMessage *message = ring_alloc_push(&session->chat_message_alloc);
    message->creation_time = creation_time;
    string32_buffer_copy_string32(message->sender_name, sender_name);
    string32_buffer_copy_string32(message->content, content);
}

void
session_rm_user(Session *session, String32 *username)
{
    // Todo: use a hashmap
    size_t rm_idx = SIZE_MAX;
    for (size_t i = 0; i < session->cur_user_count; i++) {
        if (string32_buffer_equal_string32(session->users[i].name, username)) {
            rm_idx = i;
            break;
        }
    }
    assert(rm_idx != SIZE_MAX);

    // swap users
    size_t last_idx = session->cur_user_count - 1;
    for (size_t i = rm_idx + 1; i <= last_idx; i++) {
        string32_buffer_copy_string32_buffer(session->users[i-1].name, session->users[i].name);
    }

    session->cur_user_count -= 1;
}

void
session_add_user(Session *session, String32 *username)
{
    assert(session->cur_user_count < session->max_user_count);

    User *user = &session->users[session->cur_user_count++];
    string32_buffer_copy_string32(user->name, username);
}

void
session_process_event(Session *session, OSEvent *event)
{
    Arena *frame_arena = &g_fscord.frame_arena;

    switch (event->type) {
    case OS_EVENT_KEY_PRESS: {
        u32 codepoint = event->ev.key_press.code;
        if (codepoint == '\r') {
            String32 *trans_prompt = string32_buffer_to_string32(frame_arena, session->prompt);
            send_c2s_chat_message(trans_prompt);
            string32_buffer_reset(session->prompt);
        }
        else {
            string32_buffer_edit(session->prompt, event->ev.key_press);
        }
    }
    break;

    default: {
        printf("ui_update_session did not process an event\n");
    }
    }
}

void
session_reset(Session *session)
{
    session->cur_user_count = 0;

    ring_alloc_clear(&session->chat_message_alloc);

    string32_buffer_reset(session->prompt);
}

void
session_init(Session *session)
{
    memset(session, 0, sizeof(*session));
    Arena *arena = &g_fscord.perma_arena;


    size_t max_user_count = MESSAGES_MAX_USER_COUNT;
    session->cur_user_count = 0;
    session->max_user_count = max_user_count;
    session->users = arena_push(&g_fscord.perma_arena, max_user_count * sizeof(User));
    for (size_t i = 0; i < max_user_count; i++) {
        session->users[i].name = string32_buffer_create(arena, MESSAGES_MAX_MESSAGE_LEN);
    }


    size_t max_message_count = 256;
    ChatMessage *messages = arena_push(arena, max_message_count * sizeof(*messages));
    for (size_t i = 0; i < max_message_count; i++) {
        chat_message_init(&messages[i], arena);
    }

    ring_alloc_init(&session->chat_message_alloc, sizeof(*messages), max_message_count, messages);

    session->prompt = string32_buffer_create(arena, MESSAGES_MAX_MESSAGE_LEN);
}

