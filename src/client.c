#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "network.h"

int run_client (const unsigned short server_port, const char* ip_address) {
    int client_fd = create_socket();
    if (client_fd == -1)
        return -1;

    if (connect_to_server(client_fd, server_port, ip_address) == -1)
        return -1;

    // send message
    const char* message = "This is the message sent by the client!";
    send(client_fd, message, strlen(message), 0);

    // recieve response
    char buffer[1024] = {0};
    int bytes = recv(client_fd, buffer, sizeof(buffer), 0);
    fprintf(stdout, "The server sent back:\n%s\n", buffer);    

    close(client_fd);

    return 0;
}