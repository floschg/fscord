#include "server/client_manager.h"
#include "basic/string32.h"
#include "basic/basic.h"
#include "messages/messages.h"
#include "crypto/rsa.h"
#include "os/os.h"
#include "server/client_manager.h"
#include "server/c2s_handler.h"
#include "server/s2c_sender.h"

#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/epoll.h>

static Client *
client_from_id(ClientManager *mgr, ClientId client_id)
{
    Client *client = &mgr->clients[client_id-1];
    return client;
}

static void
dealloc_client(ClientManager *mgr, ClientId id)
{
    mgr->free_client_ids[mgr->free_client_count] = id;
    mgr->free_client_count++;
}

static ClientId
alloc_client(ClientManager *mgr)
{
    if (mgr->free_client_count == 0) {
        printf("alloc_client failed, no more free spots\n");
        return 0;
    }

    mgr->free_client_count -= 1;
    ClientId client_id = mgr->free_client_ids[mgr->free_client_count];

    return client_id;
}

static void
rm_client(ClientManager *mgr, ClientId client_id)
{
    Client *client = client_from_id(mgr, client_id);


    // remove from epoll, disconnect
    OSNetSecureStreamId sstream_id = client->sstream_id;
    int fd = os_net_secure_stream_get_fd(sstream_id);
    String32 *username = string32_create_from_string32(&mgr->trans_arena, client->username);

    if (epoll_ctl(mgr->epoll_fd, EPOLL_CTL_DEL, fd, 0) == -1) {
        perror("epoll_ctl<del>:");
    }
    os_net_secure_stream_close(client->sstream_id);


    // deinit
    client->sstream_id = 0;
    client->recv_buff_size_used = 0;
    memset(client->recv_buff, 0, sizeof(client->recv_buff));
    memset(client->username_buff, 0, sizeof(client->username_buff));


    // free
    dealloc_client(mgr, client_id);


    // broadcast disconnect message
    if (username->len) {
        for (size_t i = 0; i < mgr->max_client_count; i++) {
            Client *it_client = mgr->clients + i;
            if (it_client->username->len > 0) {
                send_s2c_user_update(it_client, username, S2C_USER_UPDATE_OFFLINE);
            }
        }

        printf("<");
        string32_print(username);
        printf("> disconnected\n");
    }
}

static void
add_client(ClientManager *mgr, OSNetSecureStreamId sstream_id)
{
    // allocate
    ClientId client_id = alloc_client(mgr);
    if (!client_id) {
        return;
    }


    // add to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.u32 = client_id;

    int fd = os_net_secure_stream_get_fd(sstream_id);
    if (epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1) {
        dealloc_client(mgr, client_id);
        perror("epoll_ctl<add>:");
        return;
    }


    // init client
    Client *client = client_from_id(mgr, client_id);
    client->sstream_id = sstream_id;
    assert(client->recv_buff_size_used == 0);


    return;
}


static void
handle_client_event(ClientManager *mgr, struct epoll_event event)
{
    ClientId client_id = event.data.u32;
    Client *client = client_from_id(mgr, client_id);
    u32 sstream_id = client->sstream_id;


    OSNetSecureStreamStatus status = os_net_secure_stream_get_status(sstream_id);
    if (status == OS_NET_SECURE_STREAM_ERROR ||
        status == OS_NET_SECURE_STREAM_DISCONNECTED) {
        rm_client(mgr, client_id);
        return;
    }


    int fd = os_net_secure_stream_get_fd(client->sstream_id);

    if (event.events & EPOLLERR) {
        printf("EPOLLERR occured for client_id = %d, fd = %d\n", client_id, fd);
        rm_client(mgr, client_id);
    }
    else if (event.events & EPOLLHUP) {
        printf("EPOLLHUP occured for client_id = %d, fd = %d\n", client_id, fd);
        rm_client(mgr, client_id);
    }
    else if (event.events & EPOLLIN) {
        if (!handle_c2s(mgr, client)) {
            rm_client(mgr, client_id);
            return;
        }
    }
    else {
        printf("EPOLL??? (%d) occured for client_id = %d, fd = %d\n", event.events, client_id, fd);
        rm_client(mgr, client_id);
    }
}

static void
handle_listener_event(ClientManager *mgr, struct epoll_event event)
{
    if (event.events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
        printf("listener failed\n");
        exit(0);
    }
    if (event.events & EPOLLIN) {
        OSNetSecureStreamId sstream_id = os_net_secure_stream_accept(mgr->listening_sstream_id);
        if (!sstream_id) {
            return;
        }

        add_client(mgr, sstream_id);
    }
    else {
        printf("unhandled listener event %d\n", event.events);
    }
}

bool
client_manager_handle_events(ClientManager *mgr)
{
    arena_clear(&mgr->trans_arena);

    struct epoll_event events[mgr->max_client_count];
    i32 event_count = epoll_wait(mgr->epoll_fd, events, ARRAY_COUNT(events), -1);
    if (event_count < -1) {
        perror("epoll_wait:");
        return false;
    }

    for (size_t i = 0; i < event_count; i++) {
        if (events[i].data.u64 & 1ULL<<63) {
            handle_listener_event(mgr, events[i]);
        } else {
            handle_client_event(mgr, events[i]);
        }
    }
    return true;
}

void
client_manager_deinit(ClientManager *mgr)
{
    if (mgr->epoll_fd != -1) {
        close(mgr->epoll_fd);
        mgr->epoll_fd = -1;
    }
    if (mgr->listening_sstream_id) {
        os_net_secure_stream_close(mgr->listening_sstream_id);
        mgr->listening_sstream_id = 0;
    }
    if (mgr->rsa_pri) {
        rsa_destroy(mgr->rsa_pri);
        mgr->rsa_pri = 0;
    }
}

bool
client_manager_init(ClientManager *mgr, u16 port, u32 max_client_count, Arena *arena)
{
    mgr->rsa_pri = rsa_create_via_file(arena, "./server_rsa_pri.pem", false);
    if (!mgr->rsa_pri) {
        client_manager_deinit(mgr);
        return false;
    }

    mgr->epoll_fd = epoll_create(max_client_count); 
    if (mgr->epoll_fd < 0) {
        perror("epoll_create:");
        client_manager_deinit(mgr);
        return false;
    }

    os_net_secure_streams_init(arena, max_client_count + 1);
    mgr->listening_sstream_id = os_net_secure_stream_listen(port, mgr->rsa_pri);
    if (mgr->listening_sstream_id == 0) {
        client_manager_deinit(mgr);
        return false;
    }
    int listening_fd = os_net_secure_stream_get_fd(mgr->listening_sstream_id);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.u64 = mgr->listening_sstream_id | 1ULL << 63;
    if (epoll_ctl(mgr->epoll_fd, EPOLL_CTL_ADD, listening_fd, &ev) == -1) {
        perror("epoll_ctl<add> for listening_sstream_id:");
        client_manager_deinit(mgr);
        return false;
    }


    mgr->max_client_count = max_client_count;
    mgr->free_client_count = max_client_count;

    mgr->clients = arena_push(arena, max_client_count * sizeof(Client));
    memset(mgr->clients, 0, max_client_count * sizeof(Client));
    for (size_t i = 0; i < max_client_count; i++) {
        mgr->clients[i].username = (String32*)mgr->clients[i].username_buff;
    }

    mgr->free_client_ids = arena_push(arena, max_client_count * sizeof(ClientId));
    for (size_t i = 0; i < max_client_count; i++) {
        mgr->free_client_ids[i] = i + 1;
    }


    s2c_sender_init(arena);


    arena_init(&mgr->trans_arena, KIBIBYTES(1)-64);


    printf("listening on port %d\n", port);
    return true;
}

