#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "containers/text_message.h"
#include "networking/os_networking.h"
#include "networking/socket_commands.h"
#include "networking/data_pipes.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "events/server_events.h"
#include "utils/logging.h"

#include "server.h"

int server_event_loop(poll_list* p_list, sockbuf_list* sbuf_list) {
    // poll indefinitely
    const int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        log_network_error("Poll failed");
        return EXIT_FAILURE;
    }
    const unsigned short server_event = p_list->fds[0].revents;

    // check for socket errors or disconnects
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Server socket error or disconnect detected. Disconnecting...");
        return 3;
    }
    // server recieve -> connect user
    if (server_event & POLLIN)
        if (server_connect_event_handler(p_list, sbuf_list) == EXIT_FAILURE)
            return EXIT_FAILURE;

    // loop through sockets
    for (size_t i = 1; i < p_list->size; i++) {

        const unsigned short client_event = p_list->fds[i].revents;
        const socket_t fd = p_list->fds[i].fd;

        // handle socket errors
        if (client_event & (POLLERR | POLLNVAL | POLLHUP)) {
            sockbuf_list_remove(sbuf_list, fd);
            poll_list_remove(p_list, fd);
            
            log_error("Socket error or disconnect at fd = %d. Exiting...", fd);
            continue;
        }

        // client read -> receive data
        if (client_event & POLLIN) {
            text_message msg = {0};
            text_message_init(&msg);

            int status = read_event_handler(p_list, sbuf_list, &msg, fd);
            if (status == EXIT_SUCCESS) {
                pipe_message_to_all(sbuf_list, p_list, fd, &msg);
            }

            text_message_free(&msg);
        }
        // server write -> send message data
        if (client_event & POLLOUT)
            if (write_event_handler(sbuf_list, p_list, fd) == EXIT_FAILURE)
                continue;
    }

    return EXIT_SUCCESS;
}

int run_server(const unsigned short port) {
    winsock_init();

    socket_t socket_fd = create_socket();
    if (socket_fd == SOCKET_INVALID)
        return EXIT_FAILURE;

    if (start_server_listener(socket_fd, port) == EXIT_FAILURE) 
        return EXIT_FAILURE;

    // setup polling with server fd on index 0
    poll_list p_list;
    poll_list_init(&p_list);
    poll_list_add(&p_list, socket_fd, POLLIN | POLLOUT);

    // buffer list
    sockbuf_list sbuf_list;
    sockbuf_list_init(&sbuf_list);

    // event loop implementation
    while (1) {
        const int status = server_event_loop(&p_list, &sbuf_list);

        if (status == EXIT_FAILURE) 
            continue;
        else if (status == 3)
            break;
    }

    poll_list_free(&p_list);
    sockbuf_list_free(&sbuf_list);
    socket_close(socket_fd);

    winsock_cleanup();

    return EXIT_SUCCESS;
}