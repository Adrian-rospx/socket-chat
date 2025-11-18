#ifndef SOCKET_BUFFER_H
#define SOCKET_BUFFER_H

#include <stdio.h>

#include "os_networking.h"

// byte buffer for async io operations
typedef struct {
    socket_t fd;
    
    uint8_t* incoming_buffer;
    size_t incoming_length;
    size_t incoming_capacity;

    uint8_t* outgoing_buffer;
    size_t outgoing_length;
    size_t outgoing_capacity;

    // Length prefix
    size_t exp_msg_len;
    // Flag: has the message length been read?
    int has_length;
} socket_buffer;

/* intialise socket buffer values */
int socket_buffer_init(socket_buffer* s_buf, const socket_t fd);

/* Queue message for sending to destination */
int socket_buffer_queue_outgoing(socket_buffer* s_buf, uint8_t* data, size_t length);

/* Deque partially sent message */
int socket_buffer_deque_outgoing(socket_buffer* s_buf, ssize_t bytes);

/* Append and process new data to the incoming buffer */
int socket_buffer_append_incoming(socket_buffer* s_buf, uint8_t* data, size_t length);

/* Deque bytes from start of incoming container */
int socket_buffer_deque_incoming(socket_buffer* s_buf, ssize_t bytes);

/* Process new incoming data */
int socket_buffer_process_incoming(socket_buffer* s_buf);

/* Release socket buffer memory */
int socket_buffer_free(socket_buffer* s_buf);

#endif