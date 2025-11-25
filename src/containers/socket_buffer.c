#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os_networking.h"
#include "utils/logging.h"

#include "containers/socket_buffer.h"

#define DEF_S_BUF_ALLOC 64

int socket_buffer_init(socket_buffer* s_buf, const socket_t fd) {
    s_buf->fd = fd;
    s_buf->has_length = 0;

    s_buf->incoming_buffer = malloc(sizeof(uint8_t) * DEF_S_BUF_ALLOC);
    if (s_buf->incoming_buffer == NULL) {
        log_error("failed to allocate socket io buffer");
        return EXIT_FAILURE;
    }

    s_buf->outgoing_buffer = malloc(sizeof(uint8_t) * DEF_S_BUF_ALLOC);
    if (s_buf->outgoing_buffer == NULL) {
        log_error("failed to allocate socket io buffer");
        return EXIT_FAILURE;
    }

    s_buf->incoming_capacity = DEF_S_BUF_ALLOC;
    s_buf->outgoing_capacity = DEF_S_BUF_ALLOC;

    s_buf->incoming_length = 0;
    s_buf->outgoing_length = 0;

    return EXIT_SUCCESS;
}

int socket_buffer_queue_outgoing(socket_buffer* s_buf, uint8_t* data, size_t length) {
    const size_t new_length = s_buf->outgoing_length + length;

    // grow if needed
    if (new_length > s_buf->outgoing_capacity) {
        const size_t new_cap = 
            (new_length / DEF_S_BUF_ALLOC + 1) 
            * DEF_S_BUF_ALLOC;

        uint8_t* temp_ptr = realloc(s_buf->outgoing_buffer,
            sizeof (uint8_t) * new_cap);

        if (temp_ptr == NULL) {
            log_error("failed to reallocate socket io buffer");
            return EXIT_FAILURE;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->outgoing_capacity = new_cap;
    }

    // copy new data
    memcpy(s_buf->outgoing_buffer + s_buf->outgoing_length,
        data, length);

    s_buf->outgoing_length = new_length;
    return EXIT_SUCCESS;
}

int socket_buffer_deque_outgoing(socket_buffer* s_buf, ssize_t bytes) {
    // shift remaining bytes to the front
    memmove(s_buf->outgoing_buffer, s_buf->outgoing_buffer + bytes, 
        s_buf->outgoing_length - bytes);
    
    s_buf->outgoing_length -= bytes;

    // shrink if possible
    const size_t temp_cap = 
        (s_buf->outgoing_length / DEF_S_BUF_ALLOC + 1) * DEF_S_BUF_ALLOC;

    if (temp_cap < s_buf->outgoing_capacity) {
        uint8_t* temp_ptr = realloc(s_buf->outgoing_buffer, 
            sizeof(uint8_t) * temp_cap);

        if (temp_ptr == NULL) {
            log_error("failed to reallocate socket io buffer");
            return EXIT_FAILURE;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->outgoing_capacity = temp_cap;
    }
    return EXIT_SUCCESS;
}

int socket_buffer_append_incoming(socket_buffer* s_buf, uint8_t* data, size_t length) {
    const size_t new_length = s_buf->incoming_length + length;

    // resize if necessary
    if (new_length > s_buf->incoming_capacity) {
        const size_t new_cap = 
            (new_length / DEF_S_BUF_ALLOC + 1) 
            * s_buf->incoming_capacity;

        uint8_t* temp_ptr = realloc(s_buf->incoming_buffer,
            sizeof (uint8_t) * new_cap);

        if (temp_ptr == NULL) {
            log_error("failed to reallocate socket io buffer");
            return EXIT_FAILURE;
        }

        s_buf->incoming_buffer = temp_ptr;
        s_buf->incoming_capacity = new_cap;
    }
    
    // copy new data
    memcpy(s_buf->incoming_buffer + s_buf->incoming_length, 
        data, length);
    s_buf->incoming_length = new_length;

    return EXIT_SUCCESS;
}

int socket_buffer_deque_incoming(socket_buffer* s_buf, ssize_t bytes) {
    // shift remaining bytes to the front
    memmove(s_buf->incoming_buffer, s_buf->incoming_buffer + bytes, 
        s_buf->incoming_length - bytes);
    
    s_buf->incoming_length -= bytes;
    
    return EXIT_SUCCESS;
}

int socket_buffer_free(socket_buffer* s_buf) {
    free(s_buf->incoming_buffer);
    free(s_buf->outgoing_buffer);

    s_buf->outgoing_buffer = NULL;
    s_buf->incoming_buffer = NULL;

    s_buf->outgoing_length = 0;
    s_buf->outgoing_capacity = 0;
    s_buf->incoming_length = 0;
    s_buf->incoming_capacity = 0;

    return EXIT_SUCCESS;
}