#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "server.h"

const int port = 8765;
const int max_queued_connections = 10;

// create a non-blocking socket
int init_socket() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Socket failed!");
        close(socket_fd);
        return -1;
    }

    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return -1;
    }

    return socket_fd;
}

sockaddr_in socket_address_setup() {
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    // listen to all interfaces
    address.sin_addr.s_addr = INADDR_ANY; 
    // listen to port in network byte order
    address.sin_port = htons(port); 

    return address;
}

int bind_socket(const int socket_fd, sockaddr_in* address_p) {
    if (bind(socket_fd, (sockaddr*)address_p, sizeof(*address_p)) < 0) {
        perror("Bind failed!");
        close(socket_fd);
        return -1;
    }
    return 0;
}

int start_listener(const int socket_fd) {
    if (listen(socket_fd, max_queued_connections) < 0) {
        perror("Listen failed!");
        close(socket_fd);
        return -1;
    }
    fprintf(stdout, "Listening on port %hd\n", port);
    
    return 0;
}

int start_server(int* socket_fd_p, sockaddr_in* server_addr_p) {
    (*socket_fd_p) = init_socket();
    if (*socket_fd_p == -1) return -1;
    
    (*server_addr_p) = socket_address_setup();
    if (bind_socket(*socket_fd_p, server_addr_p) == -1) return -1;

    if (start_listener(*socket_fd_p) == -1) return -1;

    return 0;
}