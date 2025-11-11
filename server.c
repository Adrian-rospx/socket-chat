#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <string.h>
#include <stdio.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

const short port = 8080;

int main() {
    // initiate socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("Socket failed!");
        return 1;
    }
    fprintf(stdout, "Socket file descriptor: %d\n", socket_fd);

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    // listen to all interfaces
    address.sin_addr.s_addr = INADDR_ANY; 
    // listen to port in network byte order
    address.sin_port = htons(port); 

    // bind socket to address and port
    if (bind(socket_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed!");
        return 1;
    }
    fputs("Bind successful!\n", stdout);

    // listen for incoming connections
    if (listen(socket_fd, 5) < 0) {
        perror("Listen failed!");
        return 1;
    }
    fprintf(stdout, "Listening on port %hd\n", port);


    return 0;
}