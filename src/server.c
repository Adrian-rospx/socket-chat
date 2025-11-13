#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "server.h"

int handle_connection(const int socket_fd) {
    // client address
    sockaddr_in client_addr = {};
    socklen_t client_len = sizeof(client_addr);

    // accept connection
    int client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed!");
        return -1;
    }
    fputs("Client connected!\n", stdout);

    // write to screen
    char buffer[1024] = {0};
    int bytes = read(client_fd, buffer, sizeof(buffer));
    fprintf(stdout, "Recieved:\n%s\n", buffer);

    // send message
    const char* message = "Message recieved";
    send(client_fd, message, strlen(message), 0);
    close(client_fd);

    return 0;
}

int run_server() {
    // initialisation
    int socket_fd;
    sockaddr_in server_addr;
    if (start_server(&socket_fd, &server_addr) == -1) {
        close(socket_fd);
        return -1;
    }

    // client connection config
    handle_connection(socket_fd);
    handle_connection(socket_fd);

    close(socket_fd);

    return 0;
}