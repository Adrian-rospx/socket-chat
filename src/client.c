#include <stdlib.h>

#include "os_networking.h"

#include "utils/logging.h"
#include "utils/socket_commands.h"
#include "containers/poll_list.h"
#include "containers/socket_buffer.h"
#include "events/client_events.h"

#include "client.h"

int client_event_loop(poll_list* p_list, socket_buffer* sock_buf, const int timeout_ms) {
    int ret = poll(p_list->fds, 2, timeout_ms);

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

    // check for socket errors or disconnects
    if (local_client_event & (POLLERR | POLLHUP | POLLNVAL)) {
        log_error("Socket error or disconnect detected. Exiting...");
        return 3;
    }

    // handle user input
    if (local_client_event & POLLIN)
        return client_stdin_event(sock_buf, p_list);

    if (server_event & POLLOUT)
        return client_write_event(sock_buf, p_list);

    // handle server messages
    if (server_event & POLLIN) 
        return client_read_event(sock_buf);

    return EXIT_SUCCESS;
}

int run_client (const unsigned short server_port, const char* ip_address) {
    winsock_init();

    const int timeout_ms = 45000;

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
    socket_buffer sock_buf;
    socket_buffer_init(&sock_buf, server_fd);

    while (1) {
        const int status = client_event_loop(&p_list, &sock_buf, timeout_ms);

        if (status == 3) // exit
            break;
        else if (status == EXIT_FAILURE) // error
            continue;
    }

    // cleanup
    socket_buffer_free(&sock_buf);
    poll_list_free(&p_list);

    winsock_cleanup();

    return EXIT_SUCCESS;
}