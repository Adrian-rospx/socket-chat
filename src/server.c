#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "server.h"

typedef struct pollfd pollfd;

const int buffer_size = 256;

int run_server() {
    // initialisation
    int socket_fd;
    sockaddr_in server_addr;
    if (start_server(&socket_fd, &server_addr) == -1)
        return -1;

    // setup polling
    pollfd fds[10] = {0};
    pollfd server_poll = {};
    server_poll.fd = socket_fd;
    server_poll.events = POLLIN;
    fds[0] = server_poll;
    int poll_count = 1;

    char buffer[buffer_size];

    while (1) {
        // poll indefinetely
        int ret = poll(fds, 10, -1);
        if (ret < 0) {
            perror("Poll failed");
            continue;
        }

        // loop through sockets
        for (int i = 0; i < 10; i++) {
            // skip empty sockets
            if (fds[i].fd == 0) continue;
            // if it's the listening socket create a new connection
            else if (fds[i].fd == socket_fd && (fds[i].revents & POLLIN)) {
                sockaddr_in client_addr = {};
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
                if (client_fd < 0) {
                    perror("Accept failed!");
                    continue;
                }

                pollfd client_poll = {};
                client_poll.fd = client_fd;
                client_poll.events = POLLIN;

                // !weird implementation
                fds[poll_count] = client_poll;
                poll_count++;

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
                    poll_count--;
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

    }

    close(socket_fd);

    return 0;
}