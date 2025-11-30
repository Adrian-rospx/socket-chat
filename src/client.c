#include <stdlib.h>
#include <threads.h>

#include "networking/data_pipes.h"
#include "networking/os_networking.h"
#include "networking/socket_commands.h"

#include "containers/thread_queue.h"
#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "containers/text_message.h"

#include "events/server_events.h"

#include "utils/logging.h"
#include "utils/stdin_thread.h"

#include "client.h"

#define TIMEOUT_MS 30000

int client_event_loop(sockbuf_list* sbuf_l, poll_list* p_list, thread_queue* stdin_queue) {
    const socket_t server_fd = p_list->fds[0].fd;

    // stdin update
    if (!thread_queue_is_empty(stdin_queue)) {
        text_message* msg = thread_queue_pop(stdin_queue);
        
        if (msg != NULL) {
            log_event("Stdin event");
            log_extra_info("Stdin message: %.*s (length: %d)", msg->length, msg->buffer, msg->length);
    
            pipe_message_to_outgoing(sbuf_l, p_list, server_fd, msg);
    
            text_message_free(msg);
        }

    }

    // poll update
    int ret = poll(p_list->fds, 1, TIMEOUT_MS);

    if (ret == -1) {
        log_network_error("Poll error");
        return 3;
    } else if (ret == 0) {
        // time out
        log_error( "Timed out!");
        return EXIT_FAILURE;
    }

    // handling
    const unsigned short server_event = p_list->fds[0].revents;


    // check for socket errors or disconnects
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Socket error or disconnect detected. Exiting...");
        return 3;
    }

    // handle user input
    // if (local_client_event & POLLIN)
    //     return client_stdin_event(sbuf_l, p_list, server_fd);

    // handle server messages
    if (server_event & POLLIN) {
        text_message msg;
        text_message_init(&msg);

        if (read_event_handler(p_list, sbuf_l, &msg, server_fd) == EXIT_SUCCESS) {
            pipe_message_to_stdout(&msg);
        }
        
        text_message_free(&msg);
    }

    // handle writing to server
    if (server_event & POLLOUT)
        return write_event_handler(sbuf_l, p_list, server_fd);

    return EXIT_SUCCESS;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    winsock_init();

    // initialisation
    socket_t server_fd = create_socket();
    if (server_fd == SOCKET_INVALID)
        return EXIT_FAILURE;

    if (connect_client_to_server(server_fd, server_port, ip_address) == -1)
        return EXIT_FAILURE;

    // setup polling
    poll_list p_list;
    if (poll_list_init(&p_list) == EXIT_FAILURE)
        return EXIT_FAILURE;

    poll_list_add(&p_list, server_fd, POLLIN | POLLOUT);

    // setup socket buffer
    sockbuf_list sbuf_list;
    if (sockbuf_list_init(&sbuf_list) == EXIT_FAILURE) {
        poll_list_free(&p_list);
        return EXIT_FAILURE;
    }

    sockbuf_list_append(&sbuf_list, server_fd);

    // setup stdin thread
    thread_queue stdin_queue;
    thread_queue_init(&stdin_queue);

    stdin_thread_args args = {.queue = &stdin_queue};

    thrd_t stdin_thread;
    thrd_create(&stdin_thread, stdin_thread_function, &args);

    while (1) {
        const int status = client_event_loop(&sbuf_list, &p_list, &stdin_queue);

        if (status == 3) // exit
            break;
        else if (status == EXIT_FAILURE) // error
            continue;
    }

    // cleanup
    sockbuf_list_free(&sbuf_list);
    poll_list_free(&p_list);

    winsock_cleanup();

    return EXIT_SUCCESS;
}