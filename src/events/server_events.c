#include <stdio.h>
#include <stdlib.h>

#include "networking/os_networking.h"
#include "networking/data_pipes.h"
#include "networking/socket_commands.h"

#include "containers/text_message.h"
#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "utils/logging.h"

#define RECV_BUFFER_SIZE 1024

int server_connect_event_handler(poll_list* p_list, sockbuf_list* sbuf_list) {
    log_event("Connect event");

    const socket_t server_fd = p_list->fds[0].fd;

    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        log_network_error("Accept failed!");
        return EXIT_FAILURE;
    }

    // add client data
    poll_list_add(p_list, client_fd, POLLIN);
    sockbuf_list_append(sbuf_list, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", (int)client_fd);

    return EXIT_SUCCESS;
}

int read_event_handler(poll_list* p_list, sockbuf_list* sbuf_list, 
            text_message* msg, const socket_t fd) {
    pollfd* pfd = poll_list_get(p_list, fd);
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    
    if (sock_buf == NULL) {
        log_error("Can't find socket fd for reading");
        return EXIT_FAILURE;
    }
    
    log_event("Read event");
    
    // read data
    uint8_t data[RECV_BUFFER_SIZE];
    ssize_t bytes_recieved = recv(fd, (char*)data, sizeof(data) - 1, 0);
    data[bytes_recieved] = '\0';

    log_extra_info("Bytes recieved: %ld", bytes_recieved);
    
    if (bytes_recieved <= 0) {
        // handle disconnection or error
        if (bytes_recieved < 0) 
            log_error("Could not read from client");

        fprintf(stdout, "Server/client disconnected fd = %d\n", (int)fd);
        
        poll_list_remove(p_list, fd);
        sockbuf_list_remove(sbuf_list, fd);

        return EXIT_FAILURE; // exit
    }
    
    // add bytes to incoming
    if (socket_buffer_append_incoming(sock_buf,
        (uint8_t*)data, 
        bytes_recieved) == EXIT_FAILURE)
        return EXIT_FAILURE;
        
    // return text message
    int status = pipe_incoming_to_message(sock_buf, msg);

    if (status == EXIT_SUCCESS)
        return EXIT_SUCCESS;
    else if (status == EXIT_FAILURE)
        return EXIT_FAILURE;
    else
        return 2; // incomplete
}

int write_event_handler(sockbuf_list* sbuf_list, poll_list* p_list, const socket_t fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    pollfd* pfd = poll_list_get(p_list, fd);

    if (sock_buf == NULL || pfd == NULL) {
        log_error("File descriptor not found");
        return EXIT_FAILURE;
    }

    if (sock_buf->outgoing_length == 0)
        return 2;

    log_event("Write event");

    // send as many bytes as possible
    ssize_t bytes_sent = send(fd, (char*)sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0);

    if (bytes_sent == -1) {
        log_network_error("Send error");
        return EXIT_FAILURE;
    }
    log_extra_info("Bytes written: %ld", bytes_sent);

    // remove sent bytes
    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent))
        return EXIT_FAILURE;

    // remove the pollout flag when empty
    if (sock_buf->outgoing_length == 0) {
        pfd->events &= ~POLLOUT;

        log_extra_info("POLLOUT reset");
    }

    return EXIT_SUCCESS;
}
