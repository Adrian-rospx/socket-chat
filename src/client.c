#include <stdlib.h>
#include <threads.h>

#include "networking/data_pipes.h"
#include "networking/os_networking.h"
#include "networking/socket_commands.h"
#include "networking/event_handlers.h"

#include "containers/thread_queue.h"
#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "containers/text_message.h"

#include "utils/logging.h"
#include "utils/stdin_thread.h"

#include "client.h"

#define TIMEOUT_MS 30000

int client_event_loop(client_loop_data* cl_d) {
    poll_list* p_list = &cl_d->p_list;
    sockbuf_list* sbuf_l = &cl_d->sbuf_l;
    thread_queue* stdin_queue = &cl_d->stdin_queue;

    // poll update
    int ret = poll(p_list->fds, p_list->size, TIMEOUT_MS);

    if (ret == -1) {
        log_network_error("Poll error");
        return 3;
    } else if (ret == 0) {
        // time out
        log_error( "Timed out!");
        return EXIT_FAILURE;
    }

    const socket_t server_fd = p_list->fds[0].fd;
    const unsigned short server_event = p_list->fds[0].revents;

    const socket_t notify_recv_fd = p_list->fds[1].fd;
    const unsigned short notify_event = p_list->fds[1].revents;
    
    socket_event_data sockev_d = {.fd = server_fd, .msg = {0}};

    // stdin update
    if (notify_event & POLLIN) {
        // clear wakeup signal
        char buf[16];
        recv(notify_recv_fd, buf, sizeof(buf), 0);

        text_message* msg = thread_queue_pop(stdin_queue);

        if (msg != NULL) {
            log_event("Stdin event");
            log_extra_info("Stdin message (length %d): %.*s", msg->length, msg->length, msg->buffer);
    
            pipe_message_to_outgoing(sbuf_l, p_list, server_fd, msg);
    
            text_message_free(msg);
        }
    }

    // check for socket errors or disconnects
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Socket error or disconnect detected. Exiting...");
        return 3;
    }

    // handle server messages
    if (server_event & POLLIN) {
        event ev = {.type = EVENT_SOCKET_READ, .data = &sockev_d};

        on_socket_read(&ev, cl_d);
    }

    // handle writing to server
    if (server_event & POLLOUT) {
        event ev = {.type = EVENT_SOCKET_WRITE, .data = &sockev_d};

        on_socket_write(&ev, cl_d);
    }

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

    client_loop_data cl_d = {0};

    // setup polling
    if (poll_list_init(&cl_d.p_list) == EXIT_FAILURE)
        return EXIT_FAILURE;

    poll_list_add(&cl_d.p_list, server_fd, POLLIN | POLLOUT);

    // setup socket buffer
    if (sockbuf_list_init(&cl_d.sbuf_l) == EXIT_FAILURE) {
        poll_list_free(&cl_d.p_list);
        return EXIT_FAILURE;
    }

    sockbuf_list_append(&cl_d.sbuf_l, server_fd);

    // setup notification sockets
    socket_t notify_recv_fd = 0;
    socket_t notify_send_fd = 0;    
    if (setup_notifier_sockets(&notify_recv_fd, &notify_send_fd)) {
        sockbuf_list_free(&cl_d.sbuf_l);
        poll_list_free(&cl_d.p_list);
        return EXIT_FAILURE;
    }

    poll_list_add(&cl_d.p_list, notify_recv_fd, POLLIN);

    // setup stdin thread
    thread_queue_init(&cl_d.stdin_queue);

    stdin_thread_args args = {
        .queue = &cl_d.stdin_queue, 
        .notify_send_fd = notify_send_fd
    };

    thrd_t stdin_thread;
    thrd_create(&stdin_thread, stdin_thread_function, &args);

    while (1) {
        const int status = client_event_loop(&cl_d);

        if (status == 3) // exit
            break;
        else if (status == EXIT_FAILURE) // error
            continue;
    }

    // cleanup
    sockbuf_list_free(&cl_d.sbuf_l);
    poll_list_free(&cl_d.p_list);

    winsock_cleanup();

    return EXIT_SUCCESS;
}