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

int server_event_loop(socket_loop_data* sl_d) {
    poll_list* p_list = &sl_d->p_list;

    // poll indefinitely
    const int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        log_network_error("Poll failed");
        return 3;
    }

    const unsigned short server_event = p_list->fds[0].revents;
    socket_event_data ev_d = {.fd = p_list->fds[0].fd, .msg = {0}};

    // check for socket errors or disconnects
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Server socket error or disconnect detected. Exiting...");
        return 3;
    }

    if (server_event & POLLIN) {
        event ev = {.type = EVENT_SOCKET_CONNECT, .data = &ev_d};
        on_server_connect(&ev, sl_d);
    }

    // loop through sockets
    for (size_t i = 1; i < p_list->size; i++) {

        const unsigned short client_event = p_list->fds[i].revents;
        socket_event_data event_data = {
            .fd = p_list->fds[i].fd,
            .msg = {0}
        };

        if (client_event & (POLLERR | POLLNVAL | POLLHUP)) {
            event ev = {.type = EVENT_SOCKET_DISCONNECT, .data = &event_data};
            on_socket_disconnect(&ev, sl_d);
            break;
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