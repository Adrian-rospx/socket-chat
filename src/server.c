#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "networking/event_handlers.h"
#include "networking/os_networking.h"
#include "networking/socket_commands.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "utils/logging.h"

#include "server.h"

int server_connect_event_handler(poll_list* p_list, sockbuf_list* sbuf_list) {
    log_event("Connect event");

    socket_t client_fd = connect_server_to_client(p_list->fds[0].fd);
    if (client_fd == SOCKET_INVALID)
        return EXIT_FAILURE;

    // add client data
    poll_list_add(p_list, client_fd, POLLIN);
    sockbuf_list_append(sbuf_list, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", (int)client_fd);

    return EXIT_SUCCESS;
}

int server_event_loop(socket_loop_data* sl_d) {
    poll_list* p_list = &sl_d->p_list;
    sockbuf_list* sbuf_l = &sl_d->sbuf_l;

    // poll indefinitely
    const int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        log_network_error("Poll failed");
        return 3;
    }

    const unsigned short server_event = p_list->fds[0].revents;

    // check for socket errors or disconnects
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Server socket error or disconnect detected. Exiting...");
        return 3;
    }
    // server recieve -> connect user
    if (server_event & POLLIN)
        if (server_connect_event_handler(p_list, sbuf_l) == EXIT_FAILURE)
            return EXIT_FAILURE;

    // loop through sockets
    for (size_t i = 1; i < p_list->size; i++) {

        const unsigned short client_event = p_list->fds[i].revents;
        const socket_t fd = p_list->fds[i].fd;

        socket_event_data event_data = {
            .fd = p_list->fds[i].fd,
            .msg = {0}
        };

        // handle socket errors
        if (client_event & (POLLERR | POLLNVAL | POLLHUP)) {
            sockbuf_list_remove(sbuf_l, fd);
            poll_list_remove(p_list, fd);
            
            log_error("Socket error or disconnect at fd = %d. Disconnecting...", fd);
            return EXIT_FAILURE;
        }

        if (client_event & POLLIN) {
            event ev = {.type = EVENT_SOCKET_READ, .data = &event_data};
            on_socket_read(&ev, sl_d);
        }

        if (client_event & POLLOUT) {
            event ev = {.type = EVENT_SOCKET_WRITE, .data = &event_data};
            on_socket_write(&ev, sl_d);
        }
    }

    return EXIT_SUCCESS;
}

int run_server(const unsigned short port) {
    winsock_init();

    // init
    socket_t socket_fd = create_socket();
    if (socket_fd == SOCKET_INVALID)
        return EXIT_FAILURE;

    if (start_server_listener(socket_fd, port) == EXIT_FAILURE) 
        return EXIT_FAILURE;

    socket_loop_data sl_d = {0};

    // setup polling with server fd on index 0
    poll_list_init(&sl_d.p_list);
    poll_list_add(&sl_d.p_list, socket_fd, POLLIN | POLLOUT);

    // setup buffer list
    sockbuf_list_init(&sl_d.sbuf_l);

    // event loop implementation
    while (1) {
        const int status = server_event_loop(&sl_d);

        if (status == EXIT_FAILURE) 
            continue;
        if (status == 3)
            break;
    }

    poll_list_free(&sl_d.p_list);
    sockbuf_list_free(&sl_d.sbuf_l);

    winsock_cleanup();

    return EXIT_SUCCESS;
}