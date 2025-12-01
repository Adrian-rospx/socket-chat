#ifndef NETWORK_H
#define NETWORK_H

#include "networking/os_networking.h"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

/* Creates a non-blocking IPv4 socket */
socket_t create_socket(void);

/* Starts listening for connections on the specified port */
int start_server_listener(const socket_t socket_fd, 
    const unsigned short port);

/* Opens a TCP connection between the client and the server */
int connect_client_to_server(const socket_t client_fd, 
    const unsigned short server_port,
    const char* ip_address);

int setup_notifier_sockets(socket_t* recv_fd, socket_t* send_fd);

#endif