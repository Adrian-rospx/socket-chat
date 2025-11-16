#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/socket_buffer.h"

const size_t default_s_buf_alloc = 16; 

int socket_buffer_init(int fd) {
    socket_buffer s_buf;
    
    s_buf.fd = fd;
    s_buf.has_length = 0;

    s_buf.incoming_buffer = (uint8_t*)malloc(sizeof(uint8_t*) * default_s_buf_alloc);
    if (s_buf.incoming_buffer == NULL) {
        fputs("Error: failed to allocate socket io buffer", stderr);
        return -1;
    }

    s_buf.outgoing_buffer = (uint8_t*)malloc(sizeof(uint8_t) * default_s_buf_alloc);
    if (s_buf.outgoing_buffer == NULL) {
        fputs("Error: failed to allocate socket io buffer", stderr);
        return -1;
    }

    s_buf.incoming_capacity = 16;
    s_buf.outgoing_capacity = 16;

    s_buf.incoming_length = 0;
    s_buf.outgoing_length = 0;

    return 0;
}

int socket_buffer_queue_ongoing(socket_buffer* s_buf, uint8_t* data, size_t length) {
    const size_t new_length = s_buf->outgoing_length + length;

    // grow if needed
    if (new_length > s_buf->outgoing_capacity) {
        const size_t temp_cap = 
            ((new_length) / default_s_buf_alloc + 1) 
            * s_buf->outgoing_capacity;

        uint8_t* temp_ptr = realloc(s_buf->outgoing_buffer,
            sizeof (uint8_t) * s_buf->outgoing_capacity);

        if (temp_ptr == NULL) {
            fputs("Error: failed to reallocate socket io buffer", stderr);
            return -1;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->outgoing_capacity = temp_cap;
    }

    // copy new data
    memcpy(s_buf->outgoing_buffer + s_buf->outgoing_length,
        data, length);

    s_buf->outgoing_length = new_length;
    return 0;
}

int socket_buffer_deque_ongoing(socket_buffer* s_buf, ssize_t bytes) {
    // shift remaining bytes to the front
    memmove(s_buf->outgoing_buffer, s_buf->outgoing_buffer + bytes, 
        s_buf->outgoing_length - bytes);
    
    s_buf->outgoing_length -= bytes;

    // shrink if possible
    const size_t temp_cap = 
        (s_buf->outgoing_length / default_s_buf_alloc + 1) * default_s_buf_alloc;

    if (temp_cap < s_buf->outgoing_capacity) {
        uint8_t* temp_ptr = realloc(s_buf->outgoing_buffer, 
            sizeof(uint8_t) * temp_cap);

        if (temp_ptr == NULL) {
            fputs("Error: failed to reallocate socket io buffer", stderr);
            return -1;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->outgoing_capacity = temp_cap;
    }
    return 0;
}

int socket_buffer_free(socket_buffer* s_buf) {
    free(s_buf->incoming_buffer);
    free(s_buf->outgoing_buffer);

    s_buf->outgoing_length = 0;
    s_buf->outgoing_capacity = 0;
    s_buf->incoming_length = 0;
    s_buf->incoming_capacity = 0;

    return 0;
}