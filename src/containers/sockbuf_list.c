#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "containers/socket_buffer.h"
#include "utils/logging.h"

#include "containers/sockbuf_list.h"

#define DEF_SOCKBUF_LIST_ALLOC 4

int sockbuf_list_init(sockbuf_list* sbuf_l) {
    sbuf_l->bufs = malloc(sizeof(socket_buffer) * 
        DEF_SOCKBUF_LIST_ALLOC);
    
    if (sbuf_l->bufs == NULL) {
        log_error("Could not allocate socket buffer list memory");
        return EXIT_FAILURE;
    }

    sbuf_l->size = 0;
    sbuf_l->capacity = 1;

    return EXIT_SUCCESS;
}

socket_buffer* sockbuf_list_get(const sockbuf_list* sbuf_l, const socket_t fd) {
    for (size_t i = 0; i < sbuf_l->size; i++) {
        if (sbuf_l->bufs[i].fd == fd) {
            return &sbuf_l->bufs[i];
        }
    }
    // not found
    return NULL;
}

int sockbuf_list_append(sockbuf_list* sbuf_l, const socket_t fd) {
    const size_t new_length = sbuf_l->size + 1;
    
    // grow if neccesary
    if (sbuf_l->size + 1 > sbuf_l->capacity) {
        const size_t new_cap = (new_length / DEF_SOCKBUF_LIST_ALLOC + 1) * DEF_SOCKBUF_LIST_ALLOC;

        socket_buffer* temp_ptr = realloc(sbuf_l->bufs, 
            new_cap * sizeof(socket_buffer));

        if (temp_ptr == NULL) {
            log_error("Socket buffer list allocation failed");
            return EXIT_FAILURE;
        }

        sbuf_l->bufs = temp_ptr;
        sbuf_l->capacity = new_cap;
    }

    if (socket_buffer_init(&sbuf_l->bufs[sbuf_l->size], fd) == EXIT_FAILURE)
        return EXIT_FAILURE;

    sbuf_l->size = new_length;

    return EXIT_SUCCESS;
}

int sockbuf_list_remove(sockbuf_list* sbuf_l, const socket_t fd) {
    // find index
    ssize_t index = -1;
    for (size_t i = 0; i < sbuf_l->size; i++) {
        if (sbuf_l->bufs[i].fd == fd) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        log_error("File descriptor not found");
        return EXIT_FAILURE;
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
        const size_t new_cap = (sbuf_l->size / DEF_SOCKBUF_LIST_ALLOC + 1) *
            DEF_SOCKBUF_LIST_ALLOC;
        socket_buffer* temp_ptr = realloc(sbuf_l->bufs, 
            sizeof(socket_buffer) * new_cap);

        if (temp_ptr == NULL) {
            log_error("Couldn't reallocate socket buffer list memory");
            return EXIT_FAILURE;
        }

        sbuf_l->bufs = temp_ptr;
        sbuf_l->capacity = new_cap;
    }
    return EXIT_SUCCESS;
}

int sockbuf_list_free(sockbuf_list* sbuf_l) {
    for (size_t i = 0; i < sbuf_l->size; i++) {
        socket_buffer_free(&sbuf_l->bufs[i]);
    }
    free(sbuf_l->bufs);
    sbuf_l->bufs = NULL;
    
    sbuf_l->size = 0;
    sbuf_l->capacity = 0;

    return EXIT_SUCCESS;
}