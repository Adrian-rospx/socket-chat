#include <poll.h>
#include <sys/socket.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"
#include "network.h"

typedef struct pollfd pollfd;

int server_event_loop(int server_fd, pollfd fds[], int* p_poll_count, char* buffer) {
    // poll indefinitely
    int ret = poll(fds, 10, -1);
    if (ret < 0) {
        perror("Poll failed");
        return -1;
    }

    // loop through sockets
    for (int i = 0; i < 10; i++) {
        // skip empty sockets
        if (fds[i].fd == 0) return -1;
        // if it's the listening socket create a new connection
        else if (fds[i].fd == server_fd && (fds[i].revents & POLLIN)) {
            sockaddr_in client_addr = {0};
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("Accept failed!");
                continue;
            }

            pollfd client_poll = {0};
            client_poll.fd = client_fd;
            client_poll.events = POLLIN;

            // !weird implementation
            fds[*p_poll_count] = client_poll;
            (*p_poll_count)++;

            fprintf(stdout, "New client connected with fd = %d\n", client_fd);
            
        }
        // for client sockets with data to read:
        else if (fds[i].revents & POLLIN) {
            // read data
            int bytes = recv(fds[i].fd, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                // handle disconnection or error
                fprintf(stdout, "Client disconnected: fd = %d\n", fds[i].fd);
                close(fds[i].fd);
                fds[i].fd = 0;
                    
                // !weird implementation
                (*p_poll_count)--;
            } else {
                buffer[bytes] = '\0';
                fprintf(stdout, "Recieved from fd = %d:\n%s\n", 
                    fds[i].fd, buffer
                );

                // echo back
                send(fds[i].fd, buffer, bytes, 0);
            }
        }
    }

    return 0;
}

int run_server(const unsigned short port) {
    const int max_queued_connections = 10;
    const int buffer_size = 256;

    int socket_fd = create_socket();
    if (socket_fd == -1)
        return -1;

    if (start_server_listener(socket_fd, port, max_queued_connections) == -1) 
        return -1;

    // setup polling
    pollfd fds[10] = {0};
    pollfd server_poll = {0};
    server_poll.fd = socket_fd;
    server_poll.events = POLLIN;
    fds[0] = server_poll;
    int poll_count = 1;

    char buffer[buffer_size];

    // event loop implementation
    while (1) {
        const int status = server_event_loop(socket_fd, fds, &poll_count, buffer);
        if (status == -1) continue;
    }

    close(socket_fd);

    return 0;
}