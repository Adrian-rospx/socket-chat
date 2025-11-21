#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "containers/poll_list.h"
#include "containers/socket_buffer.h"
#include "containers/test_message.h"

int pipe_incoming_to_message(socket_buffer* sock_buf, text_message* txt_msg) {
    // process new data
    if (!sock_buf->has_length) {
        if (sock_buf->incoming_length < 4)
            return 2; // not enough data

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

        if (text_message_init(txt_msg, sock_buf->incoming_buffer, 
            sock_buf->exp_msg_len) == -1)
            return -1;

        // remove message from buffer
        socket_buffer_deque_incoming(sock_buf, sock_buf->exp_msg_len);
        sock_buf->has_length = 0;

        return 0;
    }

    return 2; // incomplete data
}



int pipe_message_to_outgoing(socket_buffer* sock_buf, poll_list* p_list, 
    text_message* txt_msg) {
    if (txt_msg->length == 0)
        return 2;
    
    const size_t length = txt_msg->length;
    
    // process message
    uint8_t* msg_ptr = malloc(length * sizeof(uint8_t));
    if (msg_ptr == NULL) {
        fputs("Error: couldn't allocate message memory\n", stderr);
        return -1;
    }

    // copy message bytes
    memcpy(msg_ptr, txt_msg->buffer, length);

    const uint32_t out_prefix_net = htonl((uint32_t)length);

    // queue prefix and message to output
    socket_buffer_queue_outgoing(sock_buf, (uint8_t*)&out_prefix_net, 
        sizeof(uint32_t));
    socket_buffer_queue_outgoing(sock_buf, msg_ptr, length);

    fprintf(stdout, "Message passing pipe (length: %ld): %.*s\n", 
        length, (int)length, msg_ptr);

    free(msg_ptr);
    
    // add pollout flag to events
    pollfd* pfd = poll_list_get(p_list, sock_buf->fd);
    if (pfd == NULL) {
        fputs("Error: could not get poll list element\n", stderr);
        return -1;
    }

    fputs("POLLOUT set\n", stdout);
    pfd->events |= POLLOUT;

    return 0;
}