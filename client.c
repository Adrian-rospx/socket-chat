#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr; 

const short server_port = 8080;

int main () {
    // socket init
    int sock_client_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // server address
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // convert ip string to binary
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // connect to server
    if (connect(sock_client_fd, (sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Connect failed!");
        return 1;
    }

    // send message
    const char* message = "This is the message sent by the client!";
    send(sock_client_fd, message, strlen(message), 0);

    // recieve response
    char buffer[1024] = {0};
    int bytes = read(sock_client_fd, buffer, sizeof(buffer));
    fprintf(stdout, "The server sent back: %s\n", buffer);    

    close(sock_client_fd);

    return 0;
}