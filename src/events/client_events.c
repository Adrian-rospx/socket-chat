#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "containers/socket_buffer.h"
#include "containers/poll_list.h"
#include "events/data_operations.h"

int client_stdin_event(socket_buffer* sock_buf, poll_list* p_list) {
    char message[1024];

    ssize_t bytes = read(STDIN_FILENO, message,
        sizeof(message)-1);

    if (bytes == 0) {
        fputs("EOF on stdin. Exiting...\n", stdout);
        return 3;
    } else if (bytes < 0) {
        perror("Read stdin error");
        return -1;
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
    
    if (pipe_incoming_to_outgoing(sock_buf, p_list) == -1)
        return -1;

    return 0;
}

int client_write_event(socket_buffer* sock_buf, poll_list* p_list) {
    if (sock_buf->outgoing_length == 0)
        return 2;

    ssize_t bytes_sent = send(sock_buf->fd, sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0); 

    if (bytes_sent == -1) {
        perror("Send error");
        return -1;
    }
    fprintf(stdout, "Send Event; Bytes written: %ld\n", bytes_sent);

    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent) == -1)
        return -1;

    // remove the pollout flag when empty
    if (sock_buf->outgoing_length == 0) {
        p_list->fds[1].events &= ~POLLOUT;
    }

    return 0;
}

int client_read_event(socket_buffer* sock_buf, poll_list* p_list) {
    char data[1024];

    ssize_t bytes = recv(sock_buf->fd, data, sizeof(data) - 1, 0);
            
    if (bytes < 0) {
        perror("Recv error");
        return -1;
    } else if (bytes == 0) {
        fputs("\nServer closed the connection. Exiting...\n", stderr);
        return 3;
    }
    fputs("Read event\n", stdout);

    data[bytes] = '\0';
    fprintf(stdout, "Recieved from server: %s : %ld bytes\n", data, bytes);

    if (socket_buffer_append_incoming(sock_buf, (uint8_t*)data, bytes) == -1)
        return -1;

    if (pipe_incoming_to_outgoing(sock_buf, p_list) == -1)
        return -1;

    return 0;
}
