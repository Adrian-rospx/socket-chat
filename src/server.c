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

int init_socket() {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        perror("Socket failed!");
        return -1;
    }
    return socket_fd;
}

sockaddr_in socket_address_setup() {
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    // listen to all interfaces
    address.sin_addr.s_addr = INADDR_ANY; 
    // listen to port in network byte order
    address.sin_port = htons(port); 

    return address;
}

int bind_socket(const int socket_fd, sockaddr_in* address_p) {
    if (bind(socket_fd, (sockaddr*)address_p, sizeof(*address_p)) < 0) {
        perror("Bind failed!");
        return -1;
    }
    return 0;
}

int start_listener(const int socket_fd) {
    if (listen(socket_fd, max_connections) < 0) {
        perror("Listen failed!");
        return -1;
    }
    fprintf(stdout, "Listening on port %hd\n", port);
    
    return 0;
}

int start_server(int* socket_fd_p, sockaddr_in* server_addr_p) {
    (*socket_fd_p) = init_socket();
    if (*socket_fd_p == -1) return -1;
    
    (*server_addr_p) = socket_address_setup();
    if (bind_socket(*socket_fd_p, server_addr_p) == -1) return -1;

    if (start_listener(*socket_fd_p) == -1) return -1;

    return 0;
}

int handle_connection(const int socket_fd) {
    // client address
    sockaddr_in client_addr = {};
    socklen_t client_len = sizeof(client_addr);

    // accept connection
    int client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        perror("Accept failed!");
        return -1;
    }
    fputs("Client connected!\n", stdout);

    // write to screen
    char buffer[1024] = {0};
    int bytes = read(client_fd, buffer, sizeof(buffer));
    fprintf(stdout, "Recieved:\n%s\n", buffer);

    // send message
    const char* message = "Message recieved";
    send(client_fd, message, strlen(message), 0);
    close(client_fd);

    return 0;
}

int run_server() {
    // initialisation
    int socket_fd;
    sockaddr_in server_addr;
    if (start_server(&socket_fd, &server_addr) == -1) {
        close(socket_fd);
        return -1;
    }

    // client connection config
    handle_connection(socket_fd);
    handle_connection(socket_fd);

    close(socket_fd);

    return 0;
}