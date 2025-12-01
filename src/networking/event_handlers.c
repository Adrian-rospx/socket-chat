
#include <stdlib.h>

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "containers/text_message.h"

#include "networking/data_pipes.h"
#include "networking/os_networking.h"
#include "networking/socket_commands.h"
#include "utils/logging.h"

#include "networking/event_handlers.h"

#define WAKEUP_BUFFER_SIZE 16

int on_socket_read(event* ev, void* program_data) {
    socket_loop_data* state = program_data;
    socket_event_data* ev_d = ev->data;
    
    socket_t fd = ev_d->fd;
    socket_buffer* sock_buf = sockbuf_list_get(&state->sbuf_l, fd);

    text_message_init(&ev_d->msg);
    
    log_event("Read event");

    // send data to incoming
    int res = pipe_recieve_to_incoming(&state->sbuf_l, ev_d->fd);
    
    if (res == EXIT_FAILURE) {
        text_message_free(&ev_d->msg);
        return EXIT_FAILURE;
    } else if (res == 4) {
        event ev = {.type = EVENT_SOCKET_DISCONNECT, .data = ev_d};
        on_socket_disconnect(&ev, program_data);
        return 4; // exit
    }

    // return text message
    if (pipe_incoming_to_message(sock_buf, &ev_d->msg) != EXIT_SUCCESS) {
        text_message_free(&ev_d->msg);
        return EXIT_FAILURE;
    }

    pipe_message_to_stdout(&ev_d->msg);

    text_message_free(&ev_d->msg);

    return EXIT_SUCCESS;
}

int on_socket_write(event* ev, void* program_data) {
    socket_loop_data* state = program_data;
    socket_event_data* se_d = ev->data;

    if (pipe_outgoing_to_send(&state->sbuf_l, &state->p_list, se_d->fd) == EXIT_FAILURE)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

int on_stdin_read(event* ev, void* program_data) {
    client_loop_data* state = program_data;
    client_event_data* se_d = ev->data;

    // clear wakeup signal
    char buf[WAKEUP_BUFFER_SIZE];
    recv(se_d->notifier_fd, buf, sizeof(buf), 0);

    text_message* msg = thread_queue_pop(&state->stdin_queue);

    if (msg == NULL) {
        return EXIT_FAILURE;
    }

    log_event("Stdin event");
    log_extra_info("Stdin message (length %d): %.*s", msg->length, msg->length, msg->buffer);

    int res = pipe_message_to_outgoing(&state->sbuf_l, &state->p_list, se_d->fd, msg);
    
    if (res == EXIT_FAILURE) {
        text_message_free(msg);
        return EXIT_FAILURE;
    }

    text_message_free(msg);

    return EXIT_SUCCESS;
}

int on_socket_disconnect(event* ev, void* program_data) {
    socket_loop_data* state = program_data;
    socket_event_data* ev_d = ev->data;

    log_event("Disconnect event");

    sockbuf_list_remove(&state->sbuf_l, ev_d->fd);
    poll_list_remove(&state->p_list, ev_d->fd);
            
    log_error("Socket error or disconnect at fd = %d. Disconnecting...", ev_d->fd);
    
    return EXIT_SUCCESS;
}

int on_server_connect(event* ev, void* program_data) {
    socket_loop_data* state = program_data;
    socket_event_data* ev_d = ev->data;
    
    log_event("Connect event");

    socket_t client_fd = connect_server_to_client(ev_d->fd);
    if (client_fd == SOCKET_INVALID)
        return EXIT_FAILURE;

    // add client data
    poll_list_add(&state->p_list, client_fd, POLLIN);
    sockbuf_list_append(&state->sbuf_l, client_fd);

    fprintf(stdout, "Client connected fd = %d\n", (int)client_fd);

    return EXIT_SUCCESS;
}
