#include <memory.h>
#include <stddef.h>
#include <stdint.h>
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
        uint32_t raw_prefix_net = 0;
        memcpy(&raw_prefix_net, sock_buf->incoming_buffer, sizeof(uint32_t));
        const uint32_t message_length = ntohl(raw_prefix_net);

        sock_buf->exp_msg_len = message_length;
        sock_buf->has_length = 1;

        /* Remove prefix bytes from incoming */
        socket_buffer_deque_incoming(sock_buf, sizeof(uint32_t));
    }

    if (sock_buf->has_length && sock_buf->incoming_length >= sock_buf->exp_msg_len) {
        size_t length = sock_buf->exp_msg_len;

        // process message
        uint8_t* msg = malloc((length + 1) * sizeof(uint8_t));
        if (msg == NULL) {
            fputs("Error: couldn't allocate message memory", stderr);
            return -1;
        }

        // copy message bytes
        memcpy(msg, sock_buf->incoming_buffer, length);
        msg[length] = '\0';

        const uint32_t out_prefix_net = htonl((uint32_t)length);

        // queue prefix and message to output
        socket_buffer_queue_outgoing(sock_buf, (uint8_t*)&out_prefix_net, 
            sizeof(uint32_t));
        socket_buffer_queue_outgoing(sock_buf, msg, length);

        fprintf(stdout, "Message passing pipe (length: %ld): %s\n", length, msg);

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
