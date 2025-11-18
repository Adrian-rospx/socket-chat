#ifndef NETWORK_H
#define NETWORK_H

#include <sys/poll.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "os_networking.h"

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct pollfd pollfd;

/* Creates a non-blocking IPv4 socket */
int create_socket(void);

/* Starts listening for connections on the specified port */
int start_server_listener(const socket_t socket_fd, 
    const unsigned short port, 
    const int max_queued_connections);

/* Opens a TCP connection between the client and the server */
int connect_client_to_server(const socket_t client_fd, 
    const unsigned short server_port,
    const char* ip_address);

#endif