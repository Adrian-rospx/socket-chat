#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "os_networking.h"

#include "utils/socket_commands.h"

socket_t create_socket(void) {
    // create an ipv4 socket
    socket_t socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Socket failed!");
        close(socket_fd);
        return SOCKET_INVALID;
    }

    // setup non-blocking flag
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        return SOCKET_INVALID;
    }
    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL");
        return SOCKET_INVALID;
    }

    return socket_fd;
}

int start_server_listener(socket_t socket_fd, unsigned short port, 
    const int max_queued_connections) {
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

int connect_client_to_server(const socket_t socket_fd, const unsigned short server_port, 
    const char* ip_address) {
    // set server address
    sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    // convert ip string to binary
    inet_pton(AF_INET, ip_address, &server_addr.sin_addr);
    
    // connect to server
    if (connect(socket_fd, (sockaddr*)&server_addr, 
        sizeof(server_addr)) != 0) {
        if (errno != EINPROGRESS) {
            perror("Connect failed!");
            return -1;
        }

        pollfd pfd;
        pfd.fd = socket_fd;
        pfd.events = POLLOUT;

        int ret = poll(&pfd, 1, 5000);

        if (ret == 0) {
            fputs("Error: connection timeout\n", stderr);
            close(socket_fd);
            return -1;
        } else if (ret < 0) {
            perror("Error during poll");
            close(socket_fd);
            return -1;
        }

        // check for connection errors
        int error = 0;
        socklen_t len = sizeof(error);

        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            perror("getsockopt failed");
            close(socket_fd);
            return -1;
        }

        if (error != 0) {
            fprintf(stderr, "Error: connection failed. %s\n", strerror(error));
            close(socket_fd);
            return -1;
        }
    }
    fputs("Connection established\n", stdout);
    return 0;
}
