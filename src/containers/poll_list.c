#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "containers/poll_list.h"
#include "os_networking.h"

#define DEF_POLL_LIST_ALLOC 4

int poll_list_init(poll_list* plist) {
    plist->fds = (pollfd*)malloc(sizeof(pollfd) * DEF_POLL_LIST_ALLOC);
    if (plist->fds == NULL) {
        fputs("Error: malloc fail\n", stderr);
        return -1;
    }

    plist->size = 0;
    plist->capacity = DEF_POLL_LIST_ALLOC;

    return 0;
}

int poll_list_add(poll_list* plist, const socket_t fd, const short events) {
    // increase capacity if needed
    if (plist->size + 1 > plist->capacity) {
        pollfd* temp = (pollfd*)realloc(plist->fds,
            sizeof(pollfd) * (plist->capacity + DEF_POLL_LIST_ALLOC));
        if (temp == NULL) {
            fputs("Error: realloc add fail\n", stderr);
            return -1;
        }

        plist->fds = temp;
        plist->capacity += 4;
    }

    const pollfd new_fd = {fd, events, 0};
    
    // add new file descriptor element
    plist->fds[plist->size] = new_fd;
    plist->size++;

    return 0;
}

/* Get the pollfd type at the specified fd in the poll list */
pollfd* poll_list_get(poll_list* plist, const socket_t fd) {
    for (size_t i = 0; i < plist->size; i++) {
        if (plist->fds[i].fd == fd) 
            return &plist->fds[i];
    }
    return NULL;

}

int poll_list_remove(poll_list* plist, const socket_t fd) {
    // find index of fd
    ssize_t index = -1;
    for (size_t i = 0; i < plist->size; i++) {
        if (plist->fds[i].fd == fd) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        fputs("Error: file descriptor not found\n", stderr);
        return -1;
    }
    
    // move all next fds
    for (size_t i = index; i+1 < plist->size; i++) {
        const pollfd temp = plist->fds[i];
        plist->fds[i] = plist->fds[i + 1];
        plist->fds[i + 1] = temp;
    }

    socket_close(fd);
    plist->size--;

    // resize array if possible
    if (plist->size % DEF_POLL_LIST_ALLOC == 0) {
        const size_t new_cap = (plist->size / DEF_POLL_LIST_ALLOC + 1) *
            DEF_POLL_LIST_ALLOC;
        pollfd* temp_ptr = realloc(plist->fds, sizeof(pollfd) * new_cap);

        if (temp_ptr == NULL) {
            fputs("Error: couldn't realloc poll list memory\n", stderr);
            return -1;
        }

        plist->fds = temp_ptr;
        plist->capacity = new_cap;
    }

    return 0;
}

int poll_list_free(poll_list* plist) {
    for (size_t i = 0; i < plist->size; i++) {
        socket_close(plist->fds[i].fd);
    }
    free(plist->fds);
    plist->capacity = 0;
    plist->size = 0;

    return 0;
}