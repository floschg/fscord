#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <basic/basic.h>
#include <basic/string32.h>
#include <os/os.h>
#include <messages/messages.h>



typedef uint32_t ClientId;


typedef struct {
    OSNetSecureStreamId sstream_id;

    u32 recv_buff_size_used;
    u8 recv_buff[1408];

    String32 *username;
    u8 username_buff[sizeof(String32) + MESSAGES_MAX_USERNAME_LEN * sizeof(u32)];
} Client;


typedef struct ClientManager {
    EVP_PKEY *rsa_pri;
    int epoll_fd;
    OSNetSecureStreamId listening_sstream_id;

    u32 max_client_count;
    u32 free_client_count;

    Client *clients;
    ClientId *free_client_ids;

    Arena trans_arena;
} ClientManager;


bool client_manager_init(ClientManager *mgr, u16 port, u32 max_client_count, Arena *arena);
void client_manager_deinit(ClientManager *mgr);
bool client_manager_handle_events(ClientManager *mgr);


#endif // CLIENT_MANAGER_H
