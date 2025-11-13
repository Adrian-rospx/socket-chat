#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int start_server(int* socket_fd_p, sockaddr_in* server_addr_p);

int run_server();

#endif
