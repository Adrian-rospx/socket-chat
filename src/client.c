#include <stdio.h>
#include <string.h>

#include "os_networking.h"

#include "socket_commands.h"
#include "containers/poll_list.h"
#include "containers/socket_buffer.h"


#include "client.h"

// read from the console
int client_stdin_event(socket_buffer* sock_buf) {
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
    fprintf(stdout, "msg: %s\n", message);

    uint32_t message_length = strlen(message);
    uint32_t netlen = htonl(message_length);

    socket_buffer_queue_outgoing(sock_buf, (uint8_t*)&netlen, sizeof(netlen));
    socket_buffer_queue_outgoing(sock_buf, (uint8_t*)message, message_length);

    return 0;
}

// send buffered message to the server
int client_write_event(socket_buffer* sock_buf) {
    ssize_t bytes_sent = send(sock_buf->fd, sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0); 

    if (bytes_sent == -1) {
        perror("Send error");
        return -1;
    }
    fprintf(stdout, "Log: bytes written: %ld\n", bytes_sent);

    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent) == -1)
        return -1;

    return 0;
}

// read from server
int client_read_event(pollfd p_fd, socket_buffer* sock_buf) {
    char data[128];

    ssize_t bytes = recv(p_fd.fd, data, sizeof(data) - 1, 0);
            
    if (bytes < 0) {
        perror("Recv error");
        return -1;
    } else if (bytes == 0) {
        fputs("\nServer closed the connection. Exiting...\n", stderr);
        return 3;
    }

    if (socket_buffer_append_incoming(sock_buf, (uint8_t*)data, bytes) == -1)
        return -1;

    if (socket_buffer_process_incoming(sock_buf) == -1)
        return -1;

    return 0;
}

int client_event_loop(poll_list* p_list, socket_buffer* sock_buf, const int timeout_ms) {
    int ret = poll(p_list->fds, 2, timeout_ms);

    if (ret == -1) {
        perror("Poll error");
        return -1;
    } else if (ret == 0) {
        // // handle timeout
        // const char* msg = "HEALTHCHECK";
        // ssize_t msg_len = strlen(msg);

        // ssize_t bytes = send(p_list->fds[1].fd, msg, msg_len, 0);
        // if (bytes == -1) {
        //     if (errno != EAGAIN && errno != EWOULDBLOCK) {
        //         perror("Heartbeat send error");
        //         return 3;
        //     }
        // }
        // return 0;
    }

    // handle user input
    if (p_list->fds[0].revents & POLLIN)
        return client_stdin_event(sock_buf);

    if ( (p_list->fds[1].revents & POLLOUT) && sock_buf->outgoing_length > 0)
        return client_write_event(sock_buf);

    // handle server messages
    if (p_list->fds[1].revents & POLLIN) 
        return client_read_event(p_list->fds[1], sock_buf);

    // check for socket errors or disconnects
    if (p_list->fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
        return 3;
    }
    return 0;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    const int timeout_ms = 60000;

    socket_t server_fd = create_socket();
    if (server_fd == SOCKET_INVALID)
        return SOCKET_INVALID;

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

    socket_buffer_free(&sock_buf);
    poll_list_free(&p_list);

    close(server_fd);

    return 0;
}