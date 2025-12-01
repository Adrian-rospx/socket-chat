
#include <stdlib.h>

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "containers/text_message.h"

#include "networking/data_pipes.h"
#include "networking/os_networking.h"

#include "networking/event_handlers.h"

int on_socket_read(event* ev, void* program_data) {
    socket_loop_data* state = program_data;
    socket_event_data* ev_d = ev->data;
    
    socket_t fd = ev_d->fd;
    socket_buffer* sock_buf = sockbuf_list_get(&state->sbuf_l, fd);

    text_message_init(&ev_d->msg);

    // send data to incoming
    int res = pipe_recieve_to_incoming(&state->sbuf_l, ev_d->fd);
    
    if (res == EXIT_FAILURE) {
        text_message_free(&ev_d->msg);
        return EXIT_FAILURE;
    } else if (res == 4) {
        // handle disconnection or error
        fprintf(stdout, "Server/client disconnected fd = %d\n", (int)fd);
        
        sockbuf_list_remove(&state->sbuf_l, fd);
        poll_list_remove(&state->p_list, fd);

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

int on_socket_write(event *ev, void *program_data) {
    socket_loop_data* state = program_data;
    socket_event_data* se_d = ev->data;

    if (pipe_outgoing_to_send(&state->sbuf_l, &state->p_list, se_d->fd) == EXIT_FAILURE)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}