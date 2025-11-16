#include <sys/socket.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "network.h"
#include "utils/poll_list.h"

#include "server.h"

int server_connect_event(int server_fd, poll_list* p_list) {
    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed!");
        return -1;
    }
            
    poll_list_add(p_list, client_fd, POLLIN);

    fprintf(stdout, "Client connected fd = %d\n", client_fd);

    return 0;
}

int server_read_event(poll_list* p_list, int index, char* buffer) {
    // read data
    int bytes = recv(p_list->fds[index].fd, buffer, sizeof(buffer), 0);
    
    if (bytes <= 0) {
        // handle disconnection or error
        if (bytes < 0) fputs("Error: Could not read from client\n", stderr);

        fprintf(stdout, "Client disconnected fd = %d\n", p_list->fds[index].fd);
                
        if (poll_list_remove(p_list, p_list->fds[index].fd) == 0)
            return -1;
    }
    buffer[bytes] = '\0';
    fprintf(stdout, "Recieved from fd = %d:\n%s\n", 
        p_list->fds[index].fd, buffer);

    // echo back
    send(p_list->fds[index].fd, buffer, bytes, 0);

    return 0;
}

int server_event_loop(int server_fd, poll_list* p_list, char* buffer) {
    // poll indefinitely
    int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        perror("Poll failed");
        return -1;
    }

    // loop through sockets
    for (int i = 0; i < 10; i++) {
        // listening socket -> create a new connection
        if (p_list->fds[i].fd == server_fd && (p_list->fds[i].revents & POLLIN)) {
            if (server_connect_event(server_fd, p_list) == -1)
                continue;
        }
        // client sockets -> receive data
        else if (p_list->fds[i].revents & POLLIN) {
            if (server_read_event(p_list, i, buffer) == -1)
                continue;
        }
    }

    return 0;
}

int run_server(const unsigned short port) {
    const int max_queued_connections = 10;
    const int buffer_size = 1024;

    int socket_fd = create_socket();
    if (socket_fd == -1)
        return -1;

    if (start_server_listener(socket_fd, port, max_queued_connections) == -1) 
        return -1;

    // setup polling
    poll_list p_list;
    poll_list_init(&p_list);
    poll_list_add(&p_list, socket_fd, POLLIN);

    char buffer[buffer_size];

    // event loop implementation
    while (1) {
        const int status = server_event_loop(socket_fd, &p_list, buffer);
        if (status == -1) continue;
    }

    close(socket_fd);

    return 0;
}