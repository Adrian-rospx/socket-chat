#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "networking/os_networking.h"

#include "containers/sockbuf_list.h"
#include "containers/poll_list.h"
#include "containers/socket_buffer.h"
#include "containers/text_message.h"
#include "utils/logging.h"

#define RECV_BUFFER_SIZE 1024

int pipe_incoming_to_message(socket_buffer* sock_buf, text_message* txt_msg) {
    // process new data
    if (!sock_buf->has_length) {
        if (sock_buf->incoming_length < sizeof(uint32_t))
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

        if (text_message_create(txt_msg, sock_buf->incoming_buffer, 
            sock_buf->exp_msg_len) == EXIT_FAILURE)
            return EXIT_FAILURE;

        // remove message from buffer
        socket_buffer_deque_incoming(sock_buf, sock_buf->exp_msg_len);
        sock_buf->has_length = 0;

        return EXIT_SUCCESS;
    }

    return 2; // incomplete data
}

int pipe_message_to_stdout(text_message* txt_msg) {
    if (txt_msg->length == 0)
        return 2;

    const size_t length = txt_msg->length;

    fprintf(stdout, "[server]: %.*s\n", (int)length, txt_msg->buffer);

    return EXIT_SUCCESS;
}

int pipe_message_to_outgoing(sockbuf_list* sbuf_list, poll_list* p_list, 
    socket_t fd, text_message* txt_msg) {
    if (txt_msg->length == 0)
        return 2;
    
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    pollfd* pfd = poll_list_get(p_list, fd);

    const size_t length = txt_msg->length;
    
    // process message
    uint8_t* msg_ptr = malloc(length * sizeof(uint8_t));
    if (msg_ptr == NULL) {
        log_error("Couldn't allocate message memory");
        return EXIT_FAILURE;
    }

    // copy message bytes
    memcpy(msg_ptr, txt_msg->buffer, length);

    const uint32_t out_prefix_net = htonl((uint32_t)length);

    // queue prefix and message to output
    socket_buffer_queue_outgoing(sock_buf, (uint8_t*)&out_prefix_net, 
        sizeof(uint32_t));
    socket_buffer_queue_outgoing(sock_buf, msg_ptr, length);

    log_extra_info("Message passing outgoing pipe (length %ld): %.*s", 
        length, (int)length, msg_ptr);

    free(msg_ptr);
    msg_ptr = NULL;
    
    // add pollout flag to events
    if (pfd == NULL) {
        log_error("Could not get poll list element");
        return EXIT_FAILURE;
    }

    log_extra_info("POLLOUT set");
    pfd->events |= POLLOUT;

    return EXIT_SUCCESS;
}

int pipe_message_to_all(sockbuf_list* sbuf_list, poll_list* p_list, 
            text_message* txt_msg) {

    for (size_t i = 0; i < sbuf_list->size; i++) {
        if (pipe_message_to_outgoing(sbuf_list, p_list, 
            sbuf_list->bufs[i].fd, txt_msg) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int pipe_recieve_to_incoming(sockbuf_list* sbuf_list, const socket_t fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    
    if (sock_buf == NULL) {
        log_error("Can't find socket fd for reading");
        return EXIT_FAILURE;
    }
    
    // read data
    uint8_t data[RECV_BUFFER_SIZE];
    ssize_t bytes_recieved = recv(fd, (char*)data, sizeof(data) - 1, 0);
    data[bytes_recieved] = '\0';

    log_extra_info("Bytes recieved: %ld", bytes_recieved);
    
    if (bytes_recieved <= 0) {
        if (bytes_recieved < 0) 
            log_error("Could not read from client");
        return 4;
    }
    
    // add bytes to incoming
    if (socket_buffer_append_incoming(sock_buf,
                (uint8_t*)data, 
                bytes_recieved) == EXIT_FAILURE)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

int pipe_outgoing_to_send(sockbuf_list* sbuf_list, poll_list* p_list, const socket_t fd) {
    socket_buffer* sock_buf = sockbuf_list_get(sbuf_list, fd);
    pollfd* pfd = poll_list_get(p_list, fd);

    if (sock_buf == NULL || pfd == NULL) {
        log_error("File descriptor not found");
        return EXIT_FAILURE;
    }

    if (sock_buf->outgoing_length == 0)
        return 2;

    log_event("Write event");

    // send as many bytes as possible
    ssize_t bytes_sent = send(fd, (char*)sock_buf->outgoing_buffer,
        sock_buf->outgoing_length, 0);

    if (bytes_sent == -1) {
        log_network_error("Send error");
        return EXIT_FAILURE;
    }
    log_extra_info("Bytes written: %ld", bytes_sent);

    // remove sent bytes
    if (socket_buffer_deque_outgoing(sock_buf, bytes_sent))
        return EXIT_FAILURE;

    // remove the pollout flag when empty
    if (sock_buf->outgoing_length == 0) {
        pfd->events &= ~POLLOUT;

        log_extra_info("POLLOUT reset");
    }

    return EXIT_SUCCESS;
}
