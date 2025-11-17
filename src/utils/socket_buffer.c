#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "utils/socket_buffer.h"

#define DEF_S_BUF_ALLOC 64

int socket_buffer_init(socket_buffer* s_buf, int fd) {
    s_buf->fd = fd;
    s_buf->has_length = 0;

    s_buf->incoming_buffer = (uint8_t*)malloc(sizeof(uint8_t*) * DEF_S_BUF_ALLOC);
    if (s_buf->incoming_buffer == NULL) {
        fputs("Error: failed to allocate socket io buffer", stderr);
        return -1;
    }

    s_buf->outgoing_buffer = (uint8_t*)malloc(sizeof(uint8_t) * DEF_S_BUF_ALLOC);
    if (s_buf->outgoing_buffer == NULL) {
        fputs("Error: failed to allocate socket io buffer", stderr);
        return -1;
    }

    s_buf->incoming_capacity = DEF_S_BUF_ALLOC;
    s_buf->outgoing_capacity = DEF_S_BUF_ALLOC;

    s_buf->incoming_length = 0;
    s_buf->outgoing_length = 0;

    return 0;
}

int socket_buffer_queue_outgoing(socket_buffer* s_buf, uint8_t* data, size_t length) {
    const size_t new_length = s_buf->outgoing_length + length;

    // grow if needed
    if (new_length > s_buf->outgoing_capacity) {
        const size_t new_cap = 
            ((new_length) / DEF_S_BUF_ALLOC + 1) 
            * s_buf->outgoing_capacity;

        uint8_t* temp_ptr = realloc(s_buf->outgoing_buffer,
            sizeof (uint8_t) * new_cap);

        if (temp_ptr == NULL) {
            fputs("Error: failed to reallocate socket io buffer", stderr);
            return -1;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->outgoing_capacity = new_cap;
    }

    // copy new data
    memcpy(s_buf->outgoing_buffer + s_buf->outgoing_length,
        data, length);

    s_buf->outgoing_length = new_length;
    return 0;
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
            fputs("Error: failed to reallocate socket io buffer", stderr);
            return -1;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->outgoing_capacity = temp_cap;
    }
    return 0;
}

int socket_buffer_append_incoming(socket_buffer* s_buf, uint8_t* data, size_t length) {
    const size_t new_length = s_buf->incoming_length + length;

    // resize if necessary
    if (new_length > s_buf->incoming_capacity) {
        const size_t new_cap = 
            ((new_length) / DEF_S_BUF_ALLOC + 1) 
            * s_buf->incoming_capacity;

        uint8_t* temp_ptr = realloc(s_buf->outgoing_buffer,
            sizeof (uint8_t) * new_cap);

        if (temp_ptr == NULL) {
            fputs("Error: failed to reallocate socket io buffer", stderr);
            return -1;
        }

        s_buf->outgoing_buffer = temp_ptr;
        s_buf->incoming_capacity = new_cap;
    }
    
    // copy new data
    memcpy(s_buf->incoming_buffer + s_buf->incoming_length, 
        data, length);
    s_buf->incoming_length = new_length;

    return 0;
}

int socket_buffer_deque_incoming(socket_buffer* s_buf, ssize_t bytes) {
    // shift remaining bytes to the front
    memmove(s_buf->incoming_buffer, s_buf->incoming_buffer + bytes, 
        s_buf->incoming_length - bytes);
    
    s_buf->incoming_length -= bytes;
    
    return 0;
}

int socket_buffer_process_incoming(socket_buffer* s_buf) {
    // process new data
    if (!s_buf->has_length) {
        if (s_buf->incoming_length < 4)
            return 0; // not enough data

        // register new message length
        s_buf->exp_msg_len = ntohl((uint32_t)*s_buf->incoming_buffer);
        s_buf->has_length = 1;
        // remove it from the buffer
        socket_buffer_deque_incoming(s_buf, sizeof(uint32_t));
    }

    if (s_buf->has_length && s_buf->incoming_length >= s_buf->exp_msg_len) {
        // process message
        char* msg = malloc(s_buf->exp_msg_len * sizeof(uint8_t));
        if (msg == NULL) {
            fputs("Error: couldn't allocate message memory", stderr);
            return -1;
        }

        strncpy(msg, (char*)s_buf->incoming_buffer, s_buf->exp_msg_len);
        fprintf(stdout, "Message: %s\n", msg);

        free(msg);
        // remove message from buffer
        socket_buffer_deque_incoming(s_buf, s_buf->exp_msg_len);
        s_buf->has_length = 0;
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