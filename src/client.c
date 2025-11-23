#include <stdlib.h>

#include "os_networking.h"

#include "utils/logging.h"
#include "utils/socket_commands.h"
#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "events/client_events.h"

#include "client.h"

#define TIMEOUT_MS 30000

int client_event_loop(sockbuf_list* sbuf_l, poll_list* p_list) {
    int ret = poll(p_list->fds, 2, TIMEOUT_MS);

    if (ret == -1) {
        log_network_error("Poll error");
        return EXIT_FAILURE;
    } else if (ret == 0) {
        // time out
        log_error( "Timed out!");
        return EXIT_FAILURE;
    }

    const unsigned short local_client_event = p_list->fds[0].revents;
    const unsigned short server_event = p_list->fds[1].revents;
    const socket_t local_fd = p_list->fds[0].fd;
    const socket_t server_fd = p_list->fds[1].fd;

    // check for socket errors or disconnects
    if (local_client_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Stdin error or disconnect detected. Exiting...");
        return 3;
    }
    if (server_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Socket error or disconnect detected. Exiting...");
        return 3;
    }

    // handle user input
    if (local_client_event & POLLIN)
        return client_stdin_event(sbuf_l, p_list, server_fd);

    // handle writing to server
    if (server_event & POLLOUT)
        return client_write_event(&sbuf_l->bufs[0], p_list);

    // handle server messages
    if (server_event & POLLIN) 
        return client_read_event(&sbuf_l->bufs[0]);

    return EXIT_SUCCESS;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    winsock_init();

    socket_t server_fd = create_socket();
    if (server_fd == SOCKET_INVALID)
        return EXIT_FAILURE;

    if (connect_client_to_server(server_fd, server_port, ip_address) == -1)
        return EXIT_FAILURE;

    // setup polling
    poll_list p_list;
    if (poll_list_init(&p_list) == EXIT_FAILURE)
        return EXIT_FAILURE;

    poll_list_add(&p_list, STDIN_FILENO, POLLIN);
    poll_list_add(&p_list, server_fd, POLLIN | POLLOUT);

    // setup io buffer
    sockbuf_list sbuf_list;
    sockbuf_list_init(&sbuf_list);
    sockbuf_list_append(&sbuf_list, server_fd);

    while (1) {
        const int status = client_event_loop(&sbuf_list, &p_list);

        if (status == 3) // exit
            break;
        else if (status == EXIT_FAILURE) // error
            continue;
    }

    // cleanup
    sockbuf_list_free(&sbuf_list);
    poll_list_free(&p_list);

    winsock_cleanup();

    return EXIT_SUCCESS;
}