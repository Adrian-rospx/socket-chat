#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "networking/os_networking.h"
#include "networking/socket_commands.h"
#include "utils/logging.h"

#define MAX_SERVER_CONNECTIONS 32

#define CONNECTION_TIMEOUT_MS 5000

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
        sizeof(server_addr)) != EXIT_SUCCESS) {
        if (sock_errno != ERRNO_WOULDBLOCK && sock_errno != EINPROGRESS && sock_errno != EAGAIN) {
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
            log_error("Connecting to server failed. %s\n", strerror(error));
            socket_close(socket_fd);
            return EXIT_FAILURE;
        }
    }

    fputs("Connection established\n", stdout);

    return EXIT_SUCCESS;
}

socket_t connect_server_to_client(socket_t server_fd) {
    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        log_network_error("Accept failed!");
        return SOCKET_INVALID;
    }

    return client_fd;
}

int setup_notifier_sockets(socket_t* recv_fd, socket_t* send_fd) {
    *recv_fd = socket(AF_INET, SOCK_DGRAM, 0);
    *send_fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    addr.sin_port = 0; // auto-port

    int bind_res = bind(*recv_fd, (sockaddr *)&addr, sizeof(addr));
    if (bind_res < 0) {
        log_network_error("Notifer recv bind failed");
        return EXIT_FAILURE;
    }

    // get the chosen port
    socklen_t len = sizeof(addr);
    getsockname(*recv_fd, (struct sockaddr *)&addr, &len);

    // connect the sender socket to the receiver
    int conn_res = connect(*send_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (conn_res < 0) {
        log_network_error("Notifier connection failed");
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}