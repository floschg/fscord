#ifndef S2C_SENDER_H
#define S2C_SENDER_H

#include <basic/time.h>
#include <basic/arena.h>
#include <server/client_manager.h>

void s2c_sender_init(Arena *arena);

void send_s2c_login(Client *client, u32 login_result);
void send_s2c_chat_message(Client *client, String32 *username, String32 *content, Time time);
void send_s2c_user_update(Client *client, String32 *username, u32 online_status);


#endif // S2C_SENDER_H
