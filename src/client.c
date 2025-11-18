#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>

#include "os_networking.h"

#include "utils/socket_commands.h"
#include "containers/poll_list.h"
#include "containers/socket_buffer.h"
#include "events/data_operations.h"

#include "client.h"

// read from the console
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

    message[bytes] = '\0';
    if (message[bytes - 1] == '\n')
        message[bytes - 1] = '\0';

    fprintf(stdout, "Message sent: %s\n", message);

    uint32_t message_length = strlen(message);
    uint32_t netlen = htonl(message_length);

    // prepare to write
    socket_buffer_append_incoming(sock_buf, (uint8_t*)&netlen, sizeof(netlen));
    socket_buffer_append_incoming(sock_buf, (uint8_t*)message, message_length);
    
    if (pipe_incoming_to_outgoing(sock_buf, p_list) == -1)
        return -1;

    return 0;
}

// send buffered message to the server
int client_write_event(socket_buffer* sock_buf, poll_list* p_list) {
    ssize_t bytes_sent = send(sock_buf->fd, sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0); 

    if (bytes_sent == -1) {
        perror("Send error");
        return -1;
    }
    fprintf(stdout, "Bytes written: %ld\n", bytes_sent);

    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent) == -1)
        return -1;

    // remove the pollout flag when empty
    if (sock_buf->outgoing_length == 0) {
        p_list->fds[1].events &= ~POLLOUT;
    }

    return 0;
}

// read from server
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

    data[bytes] = '\0';
    fprintf(stdout, "Recieved from server: %s : %ld bytes\n", data, bytes);

    if (socket_buffer_append_incoming(sock_buf, (uint8_t*)data, bytes) == -1)
        return -1;

    if (pipe_incoming_to_outgoing(sock_buf, p_list) == -1)
        return -1;

    return 0;
}

int client_event_loop(poll_list* p_list, socket_buffer* sock_buf, const int timeout_ms) {
    int ret = poll(p_list->fds, 2, timeout_ms);

    if (ret == -1) {
        perror("Poll error");
        return -1;
    } else if (ret == 0) {
        // time out
        fputs( "Timed out!\n", stderr);
        return -1;
    }

    const unsigned short local_client_event = p_list->fds[0].revents;
    const unsigned short server_event = p_list->fds[1].revents;

    // check for socket errors or disconnects
    if (local_client_event & (POLLERR | POLLHUP | POLLNVAL)) {
        fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
        return 3;
    }

    // handle user input
    if (local_client_event & POLLIN)
        return client_stdin_event(sock_buf, p_list);

    if (server_event & POLLOUT)
        return client_write_event(sock_buf, p_list);

    // handle server messages
    if (server_event & POLLIN) 
        return client_read_event(sock_buf, p_list);

    return 0;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    const int timeout_ms = 45000;

    socket_t server_fd = create_socket();
    if (server_fd == SOCKET_INVALID)
        return -1;

    if (connect_client_to_server(server_fd, server_port, ip_address) == -1)
        return -1;

    // setup polling
    poll_list p_list;
    if (poll_list_init(&p_list) == -1)
        return -1;

    poll_list_add(&p_list, STDIN_FILENO, POLLIN);
    poll_list_add(&p_list, server_fd, POLLIN | POLLOUT);

    // setup io buffer
    socket_buffer sock_buf;
    socket_buffer_init(&sock_buf, server_fd);

    while (1) {
        const int status = client_event_loop(&p_list, &sock_buf, timeout_ms);

        if (status == 3) // exit
            break;
        else if (status == -1) // error
            continue;
    }

    // cleanup
    socket_buffer_free(&sock_buf);
    poll_list_free(&p_list);

    return 0;
}