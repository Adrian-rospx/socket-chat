#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "os_networking.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "events/server_events.h"
#include "utils/socket_commands.h"

#include "server.h"

int server_event_loop(poll_list* p_list, sockbuf_list* sbuf_list) {
    // poll indefinitely
    const int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        perror("Poll failed");
        return -1;
    }
    const unsigned short server_event = p_list->fds[0].revents;

    // check for socket errors or disconnects
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
        return 3;
    }
    // server recieve -> connect user
    if (server_event & POLLIN)
        if (server_connect_event(p_list, sbuf_list) == -1)
            return -1;

    // loop through sockets
    for (size_t i = 1; i < p_list->size; i++) {
        const unsigned short client_event = p_list->fds[i].revents;
        const socket_t fd = p_list->fds[i].fd;

        // client read -> receive data
        if (client_event & POLLIN) {
            if (server_read_event(p_list, sbuf_list, fd) == -1)
                continue;
        }
        // server write -> send message data
        if (client_event & POLLOUT)
            if (server_write_event(sbuf_list, p_list, fd) == -1)
                continue;
    }

    return 0;
}

int run_server(const unsigned short port) {
    const int max_queued_connections = 10;

    socket_t socket_fd = create_socket();
    if (socket_fd == SOCKET_INVALID)
        return -1;

    if (start_server_listener(socket_fd, port, max_queued_connections) == -1) 
        return -1;

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
        if (status == -1) 
            continue;
        else if (status == 3)
            break;
    }

    poll_list_free(&p_list);
    sockbuf_list_free(&sbuf_list);
    close(socket_fd);

    return 0;
}