#include "os_networking.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "events/data_operations.h"
#include "utils/socket_commands.h"
#include <stdio.h>

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
    uint8_t data[1024];
    ssize_t bytes_recieved = recv(fd, data, sizeof(data) - 1, 0);
    data[bytes_recieved] = '\0';

    fprintf(stdout, "Data recieved: %s\n", data);
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
    // deliver to outgoing, set POLLOUT flag
    if (pipe_incoming_to_outgoing(sock_buf, p_list) == -1)
        return -1;

    return 0;
}

int server_write_event(sockbuf_list* sbuf_list, poll_list* p_list, const socket_t fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    pollfd* pfd = poll_list_get(p_list, fd);

    if (sock_buf == NULL || pfd == NULL) {
        fputs("Error: file descriptor not found\n", stderr);
        return -1;
    }

    if (sock_buf->outgoing_length == 0)
        return 2;

    fputs("Write event\n", stdout);

    // send as many bytes as possible
    ssize_t bytes_sent = send(fd, sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0);

    if (bytes_sent == -1) {
        perror("Send error");
        return -1;
    }
    fprintf(stdout, "Log: bytes written: %ld\n", bytes_sent);

    // remove sent bytes
    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent))
        return -1;

    // remove the pollout flag when empty
    if (sock_buf->outgoing_length == 0) {
        pfd->events &= ~POLLOUT;

        fputs("Pollout off\n", stdout);
    }

    return 0;
}
