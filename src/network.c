#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>

#include <stdio.h>
#include <unistd.h>

#include "network.h"

int create_socket(void) {
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

    return socket_fd;
}

int start_server_listener(int socket_fd, unsigned short port, const int max_queued_connections) {
    // socket address setup
    sockaddr_in address = {0};
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

    return 0;
}

int connect_to_server(const int client_fd, const unsigned short server_port, const char* ip_address) {
    // set server address
    sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    // convert ip string to binary
    inet_pton(AF_INET, ip_address, &server_addr.sin_addr);
    
    // connect to server
    if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Connect failed!");
        return -1;
    }

    return 0;
}
