#ifndef _POXIS_C_SOURCE
#define _POSIX_C_SOURCE 200809L // enable POSIX.1-2017
#endif

#include <os/os.h>
#include <basic/basic.h>
#include <basic/arena.h>
#include <basic/string32.h>

#include <mongoose/mongoose.h>

#include <crypto/rsa.h>
#include <server/client_manager.h>

#include <stdio.h>
#include <string.h>


typedef struct {
    u16 port;
} ParsedArgs; 


static b32
parse_args(ParsedArgs *args, int argc, char **argv)
{
    if (argc != 3) {
        goto format_err;
    }


    if (strcmp(argv[1], "-port") != 0) {
        goto format_err;
    }

    u16 port = atoi(argv[2]);
    if (port == 0) {
        printf("port number is invalid\n");
        return false;
    }


    args->port = port;
    return true;


format_err:
    printf("invocation format error, execpting \"-port <portnum>\"\n");
    return false;
}


void
mongoose_event_handler(struct mg_connection *conn, int ev, void *data)
{
}

void
mongoose_test(void)
{
    static const char *s_listen_on = "http://127.0.0.1:8080";
    static struct mg_mgr mgr;

    printf("Starting Mongoose listeninging on %s\n", s_listen_on);
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, s_listen_on, mongoose_event_handler, NULL);

    /*
    for (;;) {
        mg_mgr_poll(&mgr, 100);   // 100 ms poll interval
    }
    */

    mg_mgr_free(&mgr);
}

int
main(int argc, char **argv)
{
    Arena perma_arena;
    arena_init(&perma_arena, MEBIBYTES(2));


    ParsedArgs args;
    if (!parse_args(&args, argc, argv)) {
        return EXIT_FAILURE;
    }


    //mongoose_test();


    ClientManager client_manager;
    if (!client_manager_init(&client_manager, args.port, MESSAGES_MAX_USER_COUNT, &perma_arena)) {
        return EXIT_FAILURE;
    }


    bool running = true;
    while (running) {
        running = client_manager_handle_events(&client_manager);
    }


    return EXIT_SUCCESS;
}

