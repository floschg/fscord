#include <basic/basic.h>
#include <messages/messages.h>
#include <server/client_manager.h>
#include <server/c2s_handler.h>
#include <server/s2c_sender.h>
#include <stdbool.h>

static bool
is_user_already_connected(ClientManager *client_mgr, String32 *username)
{
    for (size_t i = 0; i < client_mgr->max_client_count; i++) {
        Client *client = client_mgr->clients + i;
        if (!client->username) {
            // Todo: remove this check
            continue;
        }
        if (string32_equal(client->username, username)) {
            return true;
        }
    }

    return false;
}

static void
handle_c2s_chat_message(ClientManager *client_mgr, Client *client)
{
    // Todo: verify package size
    C2S_ChatMessage *chat_message = (C2S_ChatMessage*)client->recv_buff;
    chat_message->content = (String32*)((u8*)chat_message + (size_t)chat_message->content);

    Time now = os_time_get_now();

    // Todo: make proper groups
    for (size_t i = 0; i < client_mgr->max_client_count; i++) {
        Client *it_client = client_mgr->clients + i;
        if (it_client->username->len > 0) {
            send_s2c_chat_message(it_client, client->username, chat_message->content, now);
        }
    }
}


static void
handle_c2s_login(ClientManager *client_mgr, Client *client)
{
    // init package
    C2S_Login *login = (C2S_Login*)client->recv_buff;
    login->username = (String32*)((u8*)login + (size_t)login->username);
    login->password = (String32*)((u8*)login + (size_t)login->password);


    // verify package
    if (login->username->len <= 0) {
        printf("handle_c2s_login error: username len %d is invalid\n", login->username->len);
    }
    if (login->username->len > MESSAGES_MAX_USERNAME_LEN) {
        printf("handle_c2s_login error: username len %d/%d\n", login->username->len, MESSAGES_MAX_USERNAME_LEN);
        return; // Todo: rm connection?
    }
    if (login->username->len > MESSAGES_MAX_PASSWORD_LEN) {
        printf("handle_c2s_login error: password len %d/%d\n", login->password->len, MESSAGES_MAX_PASSWORD_LEN);
        return; // Todo: rm connection?
    }
    size_t message_size = sizeof(C2S_Login) + 2*sizeof(String32) + sizeof(u32) * (login->username->len + login->password->len);
    if (message_size != client->recv_buff_size_used) {
        printf("handle_c2s_login error: message size is %zu/%d\n", message_size, client->recv_buff_size_used);
        return; // Todo: rm connection?
    }


    bool user_already_connected = is_user_already_connected(client_mgr, login->username);
    if (user_already_connected) {
        send_s2c_login(client, S2C_LOGIN_ERROR);
        return; // Todo: rm connection?
    }


    // login
    for (size_t i = 0; i < login->username->len; i++) {
        client->username->codepoints[i] = login->username->codepoints[i];
    }
    client->username->len = login->username->len;

    send_s2c_login(client, S2C_LOGIN_SUCCESS);


    // send everyone's username to client (incl itself)
    for (size_t i = 0; i < client_mgr->max_client_count; i++) {
        Client *it_client = client_mgr->clients + i;
        if (it_client->username->len > 0) {
            send_s2c_user_update(client, it_client->username, S2C_USER_UPDATE_ONLINE);
        }
    }

    // send client's user update to everyone else
    for (size_t i = 0; i < client_mgr->max_client_count; i++) {
        Client *it_client = client_mgr->clients + i;
        if (it_client == client) {
            continue;
        }
        if (it_client->username->len > 0) {
            send_s2c_user_update(it_client, client->username, S2C_USER_UPDATE_ONLINE);
        }
    }

    // Todo: make function string32_printf
    printf("<");
    string32_print(client->username);;
    printf("> connected to the server\n");
}


b32
handle_c2s(ClientManager *client_mgr, Client *client)
{
    // recv header
    if (client->recv_buff_size_used < sizeof(MessageHeader)) {
        size_t size_to_recv = sizeof(MessageHeader) - client->recv_buff_size_used;
        i64 size_recvd = os_net_secure_stream_recv(client->sstream_id, client->recv_buff + client->recv_buff_size_used, size_to_recv);
        if (size_recvd < 0) {
            return false;
        }
        else if (size_recvd == 0) {
            return false;
        }

        client->recv_buff_size_used += size_recvd;
        if (client->recv_buff_size_used < sizeof(MessageHeader)) {
            return true;
        }
    }


    // recv body
    MessageHeader *header = (MessageHeader*)client->recv_buff;
    if (client->recv_buff_size_used < header->size) {
        size_t size_to_recv = header->size - client->recv_buff_size_used;
        i64 size_recvd = os_net_secure_stream_recv(client->sstream_id, client->recv_buff + client->recv_buff_size_used, size_to_recv);
        if (size_recvd < 0) {
            return false;
        }
        else if (size_recvd == 0) {
            return false;
        }

        client->recv_buff_size_used += size_recvd;
        if (client->recv_buff_size_used < header->size) {
            return true;
        }
    }


    // dispatch
    switch (header->type) {
        case C2S_LOGIN:        handle_c2s_login(client_mgr, client);        break;
        case C2S_CHAT_MESSAGE: handle_c2s_chat_message(client_mgr, client); break;
    }


    // cleanup
    client->recv_buff_size_used = 0;
    return true;
}

