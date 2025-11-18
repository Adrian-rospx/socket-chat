#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>

#include "os_networking.h"

#include "utils/socket_commands.h"
#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "containers/socket_buffer.h"

#include "server.h"

int server_connect_event(poll_list* p_list, sockbuf_list* sbuf_list) {
    fputs("Connect event\n", stdout);

    const socket_t server_fd = p_list->fds[0].fd;

    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed!");
        return -1;
    }

    // add client data
    poll_list_add(p_list, client_fd, POLLIN);
    sockbuf_list_append(sbuf_list, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", client_fd);

    return 0;
}

int server_read_event(poll_list* p_list, sockbuf_list* sbuf_list, const socket_t fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    
    if (sock_buf == NULL) {
        fputs("Error: Can't find socket fd for reading\n", stderr);
        return -1;
    }
    
    fputs("Read event\n", stdout);
    
    // read data
    char data[128];
    ssize_t bytes_recieved = recv(fd, data, sizeof(data), 0);

    fprintf(stdout, "Bytes recieved: %ld\n", bytes_recieved);
    
    if (bytes_recieved <= 0) {
        // handle disconnection or error
        if (bytes_recieved < 0) 
            fputs("Error: Could not read from client\n", stderr);

        fprintf(stdout, "Client disconnected fd = %d\n", fd);
        
        poll_list_remove(p_list, fd);
        sockbuf_list_remove(sbuf_list, fd);

        return 0;
    }
    
    // add bytes to incoming
    if (socket_buffer_append_incoming(sock_buf,
        (uint8_t*)data, 
        bytes_recieved) == -1)
        return -1;
    // deliver to outgoing
    if (pipe_incoming_to_outgoing(sock_buf) == -1)
        return -1;

    return 0;
}

int server_message_handler(socket_buffer* sock_buf) {
    // check for message to exist
    if (sock_buf->outgoing_length == 0)
        return 2;

    fputs("Message event\n", stdout);
    
    // prepare new message
    uint8_t* message = sock_buf->outgoing_buffer;
    uint32_t message_length = sock_buf->outgoing_length;
    uint32_t netlen = htonl(message_length);

    fputs("Message prepared\n", stdout);

    socket_buffer_queue_outgoing(sock_buf, (uint8_t*)&netlen, sizeof(netlen));
    
    if (socket_buffer_queue_outgoing(sock_buf, 
        (uint8_t*)message, message_length) == -1)
        return -1;

    fputs("Message queued\n", stdout);

    return 0;
}

/* Write outgoing buffer contents */
int server_write_event(sockbuf_list* sbuf_list, const socket_t fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);

    if (sock_buf == NULL) {
        fputs("Error: file descriptor not found\n", stderr);
        return -1;
    }

    if (sock_buf->outgoing_length == 0)
        return 2;

    fputs("Write event\n", stdout);

    // send as many bytes as possible
    ssize_t bytes_sent = send(fd, sock_buf,
        sock_buf->outgoing_length, 0);

    if (bytes_sent == -1) {
        perror("Send error");
        return -1;
    }
    fprintf(stdout, "Log: bytes written: %ld\n", bytes_sent);

    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent))
        return -1;

    return 0;
}

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
        if (server_event & POLLOUT)
            if (server_write_event(sbuf_list, fd) == -1)
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