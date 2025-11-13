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

// int handle_connection(const int socket_fd) {
//     // client address
//     sockaddr_in client_addr = {};
//     socklen_t client_len = sizeof(client_addr);

//     // accept connection
//     int client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
//     if (client_fd == -1) {
//         perror("Accept failed!");
//         close(client_fd);
//         return -1;
//     }
//     fputs("Client connected!\n", stdout);

//     // write to screen
//     char buffer[1024] = {0};
//     int bytes = read(client_fd, buffer, sizeof(buffer));
//     fprintf(stdout, "Recieved:\n%s\n", buffer);

//     // send message
//     const char* message = "Message recieved";
//     send(client_fd, message, strlen(message), 0);
//     close(client_fd);

//     return 0;
// }

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

    char buffer[256];

    while (1) {
        // poll indefinetely
        int ret = poll(fds, 10, -1);
        if (ret < 0) {
            perror("Poll failed");
            continue;
        }

        // loop through sockets
        for (int i = 0; i < 10; i++) {
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

                fds[poll_count] = client_poll;
                poll_count++;

                fprintf(stdout, "New client connected with fd = %d\n", client_fd);
            }
        }

    }

    // client connection config
    // while (1) {
    //     if (handle_connection(socket_fd) == -1) return -1;
    // }

    close(socket_fd);

    return 0;
}