#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_networking.h"

#include "containers/socket_buffer.h"
#include "containers/poll_list.h"
#include "containers/test_message.h"
#include "events/data_pipes.h"
#include "utils/logging.h"

int client_stdin_event(socket_buffer* sock_buf, poll_list* p_list) {
    char message[1024];

    ssize_t bytes = read(STDIN_FILENO, message,
        sizeof(message)-1);

    if (bytes == 0) {
        log_error("EOF on stdin. Exiting...");
        return 3;
    } else if (bytes < 0) {
        log_network_error("Read stdin error");
        return EXIT_FAILURE;
    }
    fputs("Stdin event\n", stdout);

    message[bytes] = '\0';
    if (message[bytes - 1] == '\n')
        message[bytes - 1] = '\0';

    fprintf(stdout, "Message sent: %s\n", message);

    uint32_t message_length = strlen(message);
    uint32_t netlen = htonl(message_length);

    // prepare to write
    socket_buffer_append_incoming(sock_buf, (uint8_t*)&netlen, sizeof(uint32_t));
    socket_buffer_append_incoming(sock_buf, (uint8_t*)message, message_length);
    
    text_message txt_msg;
    if (pipe_incoming_to_message(sock_buf, &txt_msg) == EXIT_FAILURE)
        return EXIT_FAILURE;

    if (pipe_message_to_outgoing(sock_buf, p_list, &txt_msg) == EXIT_FAILURE)
        return EXIT_FAILURE;

    return 0;
}

int client_write_event(socket_buffer* sock_buf, poll_list* p_list) {
    if (sock_buf->outgoing_length == 0)
        return 2;

    ssize_t bytes_sent = send(sock_buf->fd, sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0); 

    if (bytes_sent == -1) {
        log_network_error("Send error");
        return EXIT_FAILURE;
    }
    fputs("Send Event\n", stdout);

    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent) == EXIT_FAILURE)
        return EXIT_FAILURE;

    // remove the pollout flag when empty
    if (sock_buf->outgoing_length == 0) {
        p_list->fds[1].events &= ~POLLOUT;
        fputs("POLLOUT reset\n", stdout);
    }

    return 0;
}

int client_read_event(socket_buffer* sock_buf) {
    char data[1024];

    ssize_t bytes = recv(sock_buf->fd, data, sizeof(data) - 1, 0);
            
    if (bytes < 0) {
        log_network_error("Recv error");
        return EXIT_FAILURE;
    } else if (bytes == 0) {
        fputs("\nServer closed the connection. Exiting...\n", stderr);
        return 3;
    }
    fputs("Read event\n", stdout);

    data[bytes] = '\0';
    fprintf(stdout, "Recieved %ld bytes from server\n", bytes);

    if (socket_buffer_append_incoming(sock_buf, (uint8_t*)data, bytes) == EXIT_FAILURE)
        return EXIT_FAILURE;

    // additionally, process it and print it to the screen
    text_message txt_msg;
    if (pipe_incoming_to_message(sock_buf, &txt_msg) != 0)
        return EXIT_FAILURE;

    pipe_message_to_stdout(&txt_msg);
    
    return 0;
}
