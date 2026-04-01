#ifndef SESSION_H
#define SESSION_H

#include <basic/arena.h>
#include <basic/string32.h>
#include <basic/time.h>
#include <os/os.h>
#include <basic/ring_alloc.h>


typedef struct {
    String32Buffer *name;
} User;

typedef struct {
    Time creation_time;
    String32Buffer *sender_name;
    String32Buffer *content;
} ChatMessage;

typedef struct {
    size_t cur_user_count;
    size_t max_user_count;
    User *users;

    ChatMessage *chat_messages;
    RingAlloc chat_message_alloc;

    String32Buffer *prompt;
} Session;


void session_init(Session *session);
void session_reset(Session *session);

void session_add_user(Session *session, String32 *username);
void session_rm_user(Session *session, String32 *username);

void session_add_chat_message(Session *session, Time creation_time, String32 *sender_name, String32 *content);

void session_process_event(Session *session, OSEvent *event);
void session_draw(Session *session);

#endif // SESSION_H
