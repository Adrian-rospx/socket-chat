#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef struct pollfd pollfd;

const int port = 8765;
const int max_queued_connections = 10;
const int buffer_size = 256;

int event_loop(int server_fd, pollfd fds[], int* p_poll_count, char* buffer) {
    // poll indefinetely
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
            sockaddr_in client_addr = {};
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                perror("Accept failed!");
                continue;
            }

            pollfd client_poll = {};
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
            int bytes = recv(fds[i].fd, buffer, buffer_size, 0);
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

int run_server() {
    // create an ipv4 socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Socket failed!");
        close(socket_fd);
        return -1;
    }

    // setup non-blocking flag
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    // socket address setup
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    // listen to all interfaces
    address.sin_addr.s_addr = INADDR_ANY; 
    // listen to port in network byte order
    address.sin_port = htons(port); 

    // bind socket
    if (bind(socket_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed!");
        close(socket_fd);
        return -1;
    }

    // start listener
    if (listen(socket_fd, max_queued_connections) < 0) {
        perror("Listen failed!");
        close(socket_fd);
        return -1;
    }
    fprintf(stdout, "Listening on port %hd\n", port);

    // setup polling
    pollfd fds[10] = {0};
    pollfd server_poll = {};
    server_poll.fd = socket_fd;
    server_poll.events = POLLIN;
    fds[0] = server_poll;
    int poll_count = 1;

    char buffer[buffer_size];

    // event loop implementation
    while (1) {
        int status = event_loop(socket_fd, fds, &poll_count, buffer);
    }

    close(socket_fd);

    return 0;
}