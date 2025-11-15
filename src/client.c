#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "network.h"

typedef struct pollfd pollfd;

int run_client (const unsigned short server_port, const char* ip_address) {
    const int timeout_ms = 5000;

    int server_fd = create_socket();
    if (server_fd == -1)
        return -1;

    if (connect_to_server(server_fd, server_port, ip_address) == -1)
        return -1;

    // setup polling
    pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    
    fds[1].fd = server_fd;
    fds[1].events = POLLIN | POLLOUT;

    char buffer[1024] = {0};

    while (1) {
        int ret = poll(fds, 2, timeout_ms);

        if (ret == -1) {
            perror("Poll error");
            return -1;
        } else if (ret == 0) {
            // handle timeout
            fputs("\n[Heartbeat] Client timeout, checking health\n", stderr);
            
            const char* msg = "Hello server sent by client";
            ssize_t msg_len = strlen(msg);

            ssize_t bytes = send(fds[1].fd, msg, msg_len, 0);

            if (bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("[Heartbeat] Send buffer full, skipping heartbeat this cycle.\n");
                    // Important: The timeout logic relies on the full INTERVAL_MS passing.
                    // To ensure this, you must reset the interval tracking, which poll handles.
                } else {
                    perror("Heartbeat send error");
                    return -1;
                }
            } else if (bytes < msg_len) {
                 printf("[Heartbeat] Sent partial data (%zd/%zd). Buffer full.\n", bytes, msg_len);
                 // In a real client, you'd buffer the remainder and retry.
            } else {
                printf("[Heartbeat] Sent default message.\n");
            }

            continue;
        }


        if (fds[1].revents & POLLIN) {
            ssize_t bytes = recv(fds[1].fd, buffer, sizeof(buffer) - 1, 0);
            
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
        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fputs("Error: socket error or disconnect detected. Exiting...\n", stderr);
            return -1;
        }
    }

    // send message
    // const char* message = "This is the message sent by the client!";
    // send(server_fd, message, strlen(message), 0);

    // // recieve response
    // char buffer[1024] = {0};
    // int bytes = recv(server_fd, buffer, sizeof(buffer), 0);
    // fprintf(stdout, "The server sent back:\n%s\n", buffer);    

    close(server_fd);

    return 0;
}