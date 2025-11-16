#include <sys/poll.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "network.h"
#include "utils/poll_list.h"
#include "utils/socket_buffer.h"

typedef struct pollfd pollfd;

// read from the console
int client_stdin_event(socket_buffer* sock_buf) {
    char data[256];
    ssize_t bytes = read(STDIN_FILENO, data,
        sizeof(data)-1);

    if (bytes == 0) {
        fputs("EOF on stdin. Exiting...\n", stdout);
        return 3;
    } else if (bytes < 0) {
        perror("Read stdin error");
        return -1;
    }
    fprintf(stdout, "Log: data read: %s\n", data);

    socket_buffer_queue_ongoing(sock_buf, (uint8_t*)data, bytes);

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

    if (socket_buffer_deque_ongoing(sock_buf, bytes_sent) == -1)
        return -1;

    fprintf(stdout, "Log: bytes written: %ld\n", bytes_sent);

    return 0;
}

// read from server
int client_read_event(pollfd p_fd, char* buffer) {
    ssize_t bytes = recv(p_fd.fd, buffer, sizeof(buffer) - 1, 0);
            
    if (bytes < 0) {
        perror("Recv error");
        return -1;
    } else if (bytes == 0) {
        fputs("\nServer closed the connection. Exiting...\n", stderr);
        return 3;
    }

    buffer[bytes] = '\0';
    fprintf(stdout, "From server: %s\n", buffer);
    return 0;
}

int client_event_loop(poll_list* p_list, char* buffer, socket_buffer* sock_buf, const int timeout_ms) {
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
        //         return -1;
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
        return client_read_event(p_list->fds[1], buffer);

    // check for socket errors or disconnects
    if (p_list->fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
        return 3;
    }
    return 0;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    const int timeout_ms = 60000;

    int server_fd = create_socket();
    if (server_fd == -1)
        return -1;

    if (connect_client_to_server(server_fd, server_port, ip_address) == -1)
        return -1;

    // setup polling
    poll_list p_list;
    if (poll_list_init(&p_list) == -1)
        return -1;

    poll_list_add(&p_list, STDIN_FILENO, POLLIN);
    poll_list_add(&p_list, server_fd, POLLIN | POLLOUT);

    char buffer[1024] = {0};

    // setup io buffer
    socket_buffer sock_buf;
    socket_buffer_init(&sock_buf, server_fd);

    while (1) {
        const int status = client_event_loop(&p_list, buffer, &sock_buf, timeout_ms);
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