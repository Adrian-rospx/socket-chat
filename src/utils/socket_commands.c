#include <WinSock2.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_networking.h"
#include "utils/logging.h"

#include "utils/socket_commands.h"

#define MAX_SERVER_CONNECTIONS 32

socket_t create_socket(void) {
    // create an ipv4 socket
    socket_t socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == SOCKET_INVALID) {
        log_network_error("Socket failed!");

        socket_close(socket_fd);
        
        return SOCKET_INVALID;
    }

    if (socket_set_nonblocking(socket_fd) == SOCKET_INVALID)
        return SOCKET_INVALID;

    // setup non-blocking flag
    return socket_fd;
}

int start_server_listener(socket_t socket_fd, unsigned short port) {
    // socket address setup
    sockaddr_in address = {0};
    address.sin_family = AF_INET;
    // listen to all interfaces
    address.sin_addr.s_addr = INADDR_ANY; 
    // listen to port in network byte order
    address.sin_port = htons(port); 

    // bind socket
    if (bind(socket_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        log_network_error("Bind failed!");
        socket_close(socket_fd);
        return EXIT_FAILURE;
    }

    // start listener
    if (listen(socket_fd, MAX_SERVER_CONNECTIONS) < 0) {
        log_network_error("Listen failed!");
        socket_close(socket_fd);
        return EXIT_FAILURE;
    }
    fprintf(stdout, "Listening on port %hd\n", port);

    return EXIT_SUCCESS;
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
            log_network_error("Connect failed!");
            return EXIT_FAILURE;
        }

        pollfd pfd;
        pfd.fd = socket_fd;
        pfd.events = POLLOUT;

        int ret = poll(&pfd, 1, 5000);

        if (ret == 0) {
            log_error("connection timeout");
            socket_close(socket_fd);
            return EXIT_FAILURE;
        } else if (ret < 0) {
            log_network_error("Error during poll");
            socket_close(socket_fd);
            return EXIT_FAILURE;
        }

        // check for connection errors
        int error = 0;
        socklen_t len = sizeof(error);

        if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
            log_network_error("getsockopt failed");
            socket_close(socket_fd);
            return EXIT_FAILURE;
        }

        if (error != 0) {
            log_error("connection failed. %s\n", strerror(error));
            socket_close(socket_fd);
            return EXIT_FAILURE;
        }
    }
    
    fputs("Connection established\n", stdout);

    return EXIT_SUCCESS;
}
