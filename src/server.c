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
const int max_connections = 5;

int run_server() {
    // initiate socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd == -1) {
        perror("Socket failed!");
        return 1;
    }
    fprintf(stdout, "Socket file descriptor: %d\n", socket_fd);

    // server address
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
    if (listen(socket_fd, max_connections) < 0) {
        perror("Listen failed!");
        return 1;
    }
    fprintf(stdout, "Listening on port %hd\n", port);

    // client address
    sockaddr_in client_addr = {};
    socklen_t client_len = sizeof(client_addr);

    // accept connection
    int client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed!");
        return 1;
    }
    fputs("Client connected!\n", stdout);

    // write to screen
    char buffer[1024] = {0};
    int bytes = read(client_fd, buffer, sizeof(buffer));
    fprintf(stdout, "Recieved: %s\n", buffer);

    // send message
    const char* message = "The server has recieved your message! Hello!";
    send(client_fd, message, strlen(message), 0);

    close(client_fd);
    close(socket_fd);

    return 0;
}