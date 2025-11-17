#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

#include "network.h"
#include "utils/poll_list.h"
#include "utils/sockbuf_list.h"

#include "server.h"

int server_connect_event(poll_list* p_list, sockbuf_list* sbuf_list, int server_fd) {
    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("Accept failed!");
        return -1;
    }

    // add client data
    poll_list_add(p_list, client_fd, POLLIN);
    sockbuf_list_append(sbuf_list, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", client_fd);

    return 0;
}

int server_read_event(poll_list* p_list, sockbuf_list* sbuf_list, int fd) {
    char data[128];
    // read data
    ssize_t bytes = recv(fd, data, sizeof(data), 0);
    
    if (bytes <= 0) {
        // handle disconnection or error
        if (bytes < 0) 
            fputs("Error: Could not read from client\n", stderr);

        fprintf(stdout, "Client disconnected fd = %d\n", fd);
        
        poll_list_remove(p_list, fd);
        sockbuf_list_remove(sbuf_list, fd);

        return 0;
    }
    data[bytes] = '\0';

    fprintf(stdout, "Recieved from fd = %d:\n%s\n", fd, data);

    // echo back
    send(fd, data, bytes, 0);

    return 0;
}

int server_event_loop(int server_fd, poll_list* p_list, sockbuf_list* sbuf_list) {
    // poll indefinitely
    int ret = poll(p_list->fds, p_list->size, -1);

    if (ret < 0) {
        perror("Poll failed");
        return -1;
    }

    // loop through sockets
    for (int i = 0; i < 10; i++) {
        // listening socket -> create a new connection
        if ((p_list->fds[i].revents & POLLIN) &&
            p_list->fds[i].fd == server_fd) {
            if (server_connect_event(p_list, sbuf_list, server_fd) == -1)
                continue;
        }
        // client sockets -> receive data
        else if (p_list->fds[i].revents & POLLIN) {
            const int fd = p_list->fds[i].fd;
            if (server_read_event(p_list, sbuf_list, fd) == -1)
                continue;
        }
    }

    return 0;
}

int run_server(const unsigned short port) {
    const int max_queued_connections = 10;

    int socket_fd = create_socket();
    if (socket_fd == -1)
        return -1;

    if (start_server_listener(socket_fd, port, max_queued_connections) == -1) 
        return -1;

    // setup polling
    poll_list p_list;
    poll_list_init(&p_list);
    poll_list_add(&p_list, socket_fd, POLLIN);

    // buffer list
    sockbuf_list sbuf_list;
    sockbuf_list_init(&sbuf_list);

    // event loop implementation
    while (1) {
        const int status = server_event_loop(socket_fd, &p_list, &sbuf_list);
        if (status == -1) 
            continue;
    }

    poll_list_free(&p_list);
    sockbuf_list_free(&sbuf_list);
    close(socket_fd);

    return 0;
}