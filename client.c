#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
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

    // connect to server
    if (connect(sock_client_fd, (sockaddr*)&server_addr, sizeof(server_addr))) {
        perror("Connect failed!");
        return 1;
    }

    // send message
    const char* message = "This is the client sent message!";
    send(sock_client_fd, message, strlen(message), 0);

    // recieve response
    char buffer[1024] = {0};
    int bytes = read(sock_client_fd, buffer, strlen(buffer));
    fprintf(stdout, "The server sent: %s\n", buffer);    

    close(sock_client_fd);

    return 0;
}