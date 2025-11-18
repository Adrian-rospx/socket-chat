#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "containers/poll_list.h"
#include "containers/socket_buffer.h"

int pipe_incoming_to_outgoing(socket_buffer* sock_buf, poll_list* p_list) {
    // process new data
    if (!sock_buf->has_length) {
        if (sock_buf->incoming_length < 4)
            return 0; // not enough data

        // register new message length
        uint32_t raw_prefix;
        memcpy(&raw_prefix, sock_buf->incoming_buffer, sizeof(uint32_t));

        const uint32_t prefix = ntohl(raw_prefix);
        fprintf(stdout, "Expected message length: %d\n", prefix);

        sock_buf->exp_msg_len = prefix;
        sock_buf->has_length = 1;

        // remove it from the buffer
        socket_buffer_deque_incoming(sock_buf, sizeof(uint32_t));
    }

    if (sock_buf->has_length && sock_buf->incoming_length >= sock_buf->exp_msg_len) {
        // process message
        uint8_t* msg = malloc((sock_buf->exp_msg_len + 1) * sizeof(uint8_t));
        if (msg == NULL) {
            fputs("Error: couldn't allocate message memory", stderr);
            return -1;
        }

        strncpy((char*)msg, (char*)sock_buf->incoming_buffer, sock_buf->exp_msg_len);
        msg[sock_buf->exp_msg_len] = '\0';

        fprintf(stdout, "Message passing: %s\n", msg);
        
        // queue to output
        socket_buffer_queue_outgoing(sock_buf, msg, sock_buf->exp_msg_len);

        free(msg);

        // remove message from buffer
        socket_buffer_deque_incoming(sock_buf, sock_buf->exp_msg_len);
        sock_buf->has_length = 0;

        // add pollout flag to events
        pollfd* pfd = poll_list_get(p_list, sock_buf->fd);
        if (pfd == NULL) {
            fputs("Error: could not get poll list element", stderr);
            return -1;
        }

        pfd->events |= POLLOUT;

        return 0;
    }

    return 0; // incomplete data
}
