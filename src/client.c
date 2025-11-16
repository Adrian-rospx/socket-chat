#include <fcntl.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "network.h"
#include "utils/socket_buffer.h"

typedef struct pollfd pollfd;

int client_stdin_event(pollfd p_fd, socket_buffer* sock_buf) {
    char data[256];
    ssize_t bytes = read(STDIN_FILENO, data,
        sizeof(data)-1);

    if (bytes == 0) {
        fputs("EOF on stdin. Exiting...", stdout);
        return 3;
    } else if (bytes < 0) {
        perror("Read stdin error");
        return -1;
    }

    socket_buffer_queue_ongoing(sock_buf, (uint8_t*)data, sizeof(data)-1);

    if (!(p_fd.revents & POLLOUT)) {
        fputs("Server not ready for writing, must implement buffering\n", stdout);
        return 0;
    }

    ssize_t bytes_sent = send(p_fd.fd, sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0); 

    if (bytes_sent == -1) {
        perror("Send error");
        return -1;
    }

    socket_buffer_deque_ongoing(sock_buf, bytes_sent);

    return 0;
}

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
    fprintf(stdout, "From server: %s", buffer);
    return 0;
}

int client_event_loop(pollfd** fds, char* buffer, socket_buffer* sock_buf, const int timeout_ms) {
    int ret = poll((*fds), 2, timeout_ms);

    if (ret == -1) {
        perror("Poll error");
        return -1;
    } else if (ret == 0) {
        // handle timeout
        const char* msg = "HEALTHCHECK";
        ssize_t msg_len = strlen(msg);

        ssize_t bytes = send((*fds)[1].fd, msg, msg_len, 0);
        if (bytes == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("Heartbeat send error");
                return -1;
            }
        }
        return 0;
    }

    // handle user input
    if ((*fds)[0].revents & POLLIN) {
        return client_stdin_event((*fds)[0], sock_buf);
    }

    // handle server messages
    if ((*fds)[1].revents & POLLIN) {
        return client_read_event((*fds)[1], buffer);
    }

    // check for socket errors or disconnects
    if ((*fds)[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
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
    pollfd* fds = (pollfd*)malloc(sizeof(pollfd) * 2);
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    fds[1].fd = server_fd;
    fds[1].events = POLLIN | POLLOUT;

    char buffer[1024] = {0};

    // setup io buffer
    socket_buffer sock_buf;
    socket_buffer_init(&sock_buf, server_fd);

    while (1) {
        const int status = client_event_loop(&fds, buffer, &sock_buf, timeout_ms);
        if (status == 3) // exit
            break;
        else if (status == -1) // error
            continue;
    }

    socket_buffer_free(&sock_buf);
    free(fds);

    close(server_fd);

    return 0;
}