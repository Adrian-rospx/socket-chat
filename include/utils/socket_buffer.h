#ifndef SOCKET_BUFFER_H
#define SOCKET_BUFFER_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// byte buffer for async io operations
typedef struct {
    int fd;
    
    uint8_t* incoming_buffer;
    size_t incoming_length;
    size_t incoming_capacity;

    uint8_t* outgoing_buffer;
    size_t outgoing_length;
    size_t outgoing_capacity;

    // Length prefix
    size_t exp_message_length;
    // Flag: has the message length been read?
    int has_length;
} socket_buffer;

/* intialise socket buffer values */
int socket_buffer_init(int fd);

/* Queue message for sending to destination */
int socket_buffer_queue_ongoing(socket_buffer* s_buf, uint8_t* data, size_t length);

/* Deque partially sent message */
int socket_buffer_deque_ongoing(socket_buffer* s_buf, ssize_t bytes);

/* Release socket buffer memory */
int socket_buffer_free(socket_buffer* s_buf);

#endif