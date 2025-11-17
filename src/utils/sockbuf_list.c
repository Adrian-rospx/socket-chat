#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils/socket_buffer.h"

#include "utils/sockbuf_list.h"

#define DEF_SOCKBUF_LIST_ALLOC 4

int sockbuf_list_init(sockbuf_list* sbuf_l) {
    sbuf_l->bufs = malloc(sizeof(socket_buffer) * 
        DEF_SOCKBUF_LIST_ALLOC);
    
    if (sbuf_l->bufs == NULL) {
        fputs("Error: could not allocate socket buffer list memory\n", stderr);
        return -1;
    }

    sbuf_l->size = 0;
    sbuf_l->capacity = 1;

    return 0;
}

socket_buffer* sockbuf_list_get(const sockbuf_list* sbuf_l, const int fd) {
    for (size_t i = 0; i < sbuf_l->size; i++) {
        if (sbuf_l->bufs[i].fd == fd) {
            return &sbuf_l->bufs[i];
        }
    }
    // not found
    return NULL;
}

int sockbuf_list_append(sockbuf_list* sbuf_l, const int fd) {
    const size_t new_length = sbuf_l->size + 1;
    
    // grow if neccesary
    if (sbuf_l->size + 1 > sbuf_l->capacity) {
        const size_t new_cap = (new_length / DEF_SOCKBUF_LIST_ALLOC + 1) * DEF_SOCKBUF_LIST_ALLOC;

        socket_buffer* temp_ptr = realloc(sbuf_l, 
            new_cap * sizeof(socket_buffer));

        if (temp_ptr == NULL) {
            fputs("Error: Socket buffer list allocation failed\n", stderr);
            return -1;
        }

        sbuf_l->bufs = temp_ptr;
        sbuf_l->capacity = new_cap;
    }

    if (socket_buffer_init(&sbuf_l->bufs[sbuf_l->size], fd) == -1)
        return -1;

    sbuf_l->size = new_length;

    return 0;
}

int sockbuf_list_remove(sockbuf_list* sbuf_l, const int fd) {
    // find index
    ssize_t index = -1;
    for (size_t i = 0; i < sbuf_l->size; i++) {
        if (sbuf_l->bufs[i].fd == fd) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        fputs("Error: file descriptor not found\n", stderr);
        return -1;
    }

    // move element to the end
    for (size_t i = index; i+1 < sbuf_l->size; i++) {
        const socket_buffer temp = sbuf_l->bufs[i];
        sbuf_l->bufs[i] = sbuf_l->bufs[i + 1];
        sbuf_l->bufs[i + 1] = temp;
    }
    
    socket_buffer_free(&sbuf_l->bufs[sbuf_l->size - 1]);
    sbuf_l->size--;

    // shrink if possible
    if (sbuf_l->size % DEF_SOCKBUF_LIST_ALLOC == 0) {
        socket_buffer* temp_ptr = realloc(sbuf_l->bufs, 
            sizeof(socket_buffer) * (sbuf_l->capacity - DEF_SOCKBUF_LIST_ALLOC));

        if (temp_ptr == NULL) {
            fputs("Error: couldn't reallocate socket buffer list memory\n", stderr);
            return -1;
        }

        sbuf_l->bufs = temp_ptr;
        sbuf_l->capacity -= DEF_SOCKBUF_LIST_ALLOC;
    }
    return 0;
}

int sockbuf_list_free(sockbuf_list* sbuf_l) {
    for (size_t i = 0; i < sbuf_l->size; i++) {
        socket_buffer_free(&sbuf_l->bufs[i]);
    }
    free(sbuf_l->bufs);
    
    sbuf_l->size = 0;
    sbuf_l->capacity = 0;

    return 0;
}