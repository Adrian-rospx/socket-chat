#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#include "network.h"
#include "utils/poll_list.h"
#include "utils/sockbuf_list.h"
#include "utils/socket_buffer.h"

#include "server.h"

int server_connect_event(poll_list* p_list, sockbuf_list* sbuf_list, int server_fd) {
    fputs("Connect event\n", stdout);
    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed!");
        return -1;
    }

    // add client data
    poll_list_add(p_list, client_fd, POLLIN | POLLOUT);
    sockbuf_list_append(sbuf_list, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", client_fd);

    return 0;
}

int server_read_event(poll_list* p_list, sockbuf_list* sbuf_list, int fd) {
    fputs("Read event\n", stdout);
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    
    // read data
    char data[128];
    ssize_t bytes_recieved = recv(fd, data, sizeof(data), 0);
    
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
    if (socket_buffer_process_incoming(sock_buf) == -1)
        return -1;

    return 0;
}

int server_message_handler(socket_buffer* sock_buf) {
    // check for message to exist
    if (!sock_buf->has_length || sock_buf->incoming_length != sock_buf->exp_msg_len)
        return 2;

    fputs("Message event\n", stdout);
    
    // prepare new message
    uint8_t* message = sock_buf->incoming_buffer;
    uint32_t message_length = sock_buf->incoming_length;
    uint32_t netlen = htonl(message_length);

    fputs("Message prepared\n", stdout);

    socket_buffer_queue_outgoing(sock_buf, (uint8_t*)&netlen, sizeof(netlen));
    
    if (socket_buffer_queue_outgoing(sock_buf, (uint8_t*)message, message_length) == -1)
        return -1;

    fputs("Message queued\n", stdout);

    return 0;
}

int server_write_event(sockbuf_list* sbuf_list, int fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
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

int server_event_loop(int server_fd, poll_list* p_list, sockbuf_list* sbuf_list) {
    // poll indefinitely
    int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        perror("Poll failed");
        return -1;
    }
    
    // loop through sockets
    for (size_t i = 0; i < p_list->size; i++) {
        const int fd = p_list->fds[i].fd;

        // listening socket -> create a new connection
        if ((p_list->fds[i].revents & POLLIN) &&
            p_list->fds[i].fd == server_fd) {
            if (server_connect_event(p_list, sbuf_list, server_fd) == -1)
                continue;
        }
        // client read -> receive data
        if (p_list->fds[i].revents & POLLIN) {
            if (server_read_event(p_list, sbuf_list, fd) == -1)
                continue;
        }
        // client write -> send data
        if (p_list->fds[i].revents & POLLOUT) {
            if (server_write_event(sbuf_list, fd) == -1)
                continue;
        }

        // handle messages
        if (fd != server_fd)
            server_message_handler(sockbuf_list_get(sbuf_list, fd));

    }
    // check for socket errors or disconnects
    if (p_list->fds[0].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
        return 3;
    }

    return 0;
}

int run_server(const unsigned short port) {
    const int max_queued_connections = 10;

    int socket_fd = create_socket();
    if (socket_fd == -1)
        return -1;

    if (start_server_listener(socket_fd, port, max_queued_connections) == -1) 
        return -1;

    // setup polling
    poll_list p_list;
    poll_list_init(&p_list);
    poll_list_add(&p_list, socket_fd, POLLIN);

    // buffer list
    sockbuf_list sbuf_list;
    sockbuf_list_init(&sbuf_list);

    // event loop implementation
    while (1) {
        const int status = server_event_loop(socket_fd, &p_list, &sbuf_list);
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