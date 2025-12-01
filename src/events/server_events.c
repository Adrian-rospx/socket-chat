#include <stdio.h>
#include <stdlib.h>

#include "networking/os_networking.h"
#include "networking/socket_commands.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "utils/logging.h"

int server_connect_event_handler(poll_list* p_list, sockbuf_list* sbuf_list) {
    log_event("Connect event");

    const socket_t server_fd = p_list->fds[0].fd;

    sockaddr_in client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    socket_t client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        log_network_error("Accept failed!");
        return EXIT_FAILURE;
    }

    // add client data
    poll_list_add(p_list, client_fd, POLLIN);
    sockbuf_list_append(sbuf_list, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", (int)client_fd);

    return EXIT_SUCCESS;
}