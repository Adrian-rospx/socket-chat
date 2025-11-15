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

typedef struct pollfd pollfd;

int client_event_loop(pollfd** fds, char* buffer, const int timeout_ms) {
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
        ssize_t bytes = read(STDIN_FILENO, buffer, sizeof(buffer)-1);

        if (bytes == 0) {
            fputs("EOF on stdin. Exiting...", stdout);
            return 3;
        } else if (bytes < 0) {
            perror("Read stdin error");
            return -1;
        }

        if (!((*fds)[1].revents & POLLOUT)) {
            fputs("Server not ready for writing, must implement buffering\n", stdout);
            return 0;
        }

        if (send((*fds)[1].fd, buffer, strlen(buffer), 0) == -1) {
            perror("Send error");
            return -1;
        }
        return 0;
    }

    // handle server messages
    if ((*fds)[1].revents & POLLIN) {
        ssize_t bytes = recv((*fds)[1].fd, buffer, sizeof(buffer) - 1, 0);
            
        if (bytes < 0) {
            perror("Recv error");
            return -1;
        } else if (bytes == 0) {
            fputs("\nServer closed the connection. Exiting...\n", stderr);
            return -1;
        }

        buffer[bytes] = '\0';
        fprintf(stdout, "From server: %s", buffer);
        return 0;
    }

    // check for socket errors or disconnects
    if ((*fds)[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
        fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
        return -1;
    }
    return 0;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    const int timeout_ms = 60000;

    int server_fd = create_socket();
    if (server_fd == -1)
        return -1;

    if (connect_to_server(server_fd, server_port, ip_address) == -1)
        return -1;

    // setup polling
    pollfd* fds = (pollfd*)malloc(sizeof(pollfd) * 2);
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    fds[1].fd = server_fd;
    fds[1].events = POLLIN | POLLOUT;

    char buffer[1024] = {0};

    while (1) {
        const int status = client_event_loop(&fds, buffer, timeout_ms);
        if (status == 3) // exit
            break;
        else if (status == -1) // error
            continue;
    }

    free(fds);

    close(server_fd);

    return 0;
}