#include <stdio.h>
#include <stdlib.h>

#include "poll_list.h"

const int DEFAULT_ALLOCATION = 4;

int poll_list_init(poll_list* plist) {
    plist->fds = (pollfd*)malloc(sizeof(pollfd) * DEFAULT_ALLOCATION);
    if (plist->fds == NULL) {
        fputs("Error: malloc fail\n", stderr);
        return -1;
    }

    plist->size = 0;
    plist->capacity = DEFAULT_ALLOCATION;

    return 0;
}

int poll_list_add(poll_list* plist, const int fd, const short events) {
    // increase capacity if needed
    if (plist->size + 1 > plist->capacity) {
        pollfd* temp = (pollfd*)realloc(plist->fds,
            sizeof(pollfd) * (plist->capacity + DEFAULT_ALLOCATION));
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

int poll_list_remove(poll_list* plist, const int fd) {
    // find index of fd
    int index = -1;
    for (int i = 0; i < plist->size; i++) {
        if (plist->fds->fd == fd) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        fputs("Error: file descriptor not found\n", stderr);
        return -1;
    }
    
    // move all next fds
    for (int i = index; i+1 < plist->size; i++) {
        const pollfd temp = plist->fds[i];
        plist->fds[i] = plist->fds[i + 1];
        plist->fds[i + 1] = temp;
    }

    // resize array if possible
    if ((plist->size - 1) % DEFAULT_ALLOCATION == 0) {
        pollfd* temp = (pollfd*)realloc(plist->fds, 
            sizeof(pollfd) * (plist->capacity - DEFAULT_ALLOCATION));

        if (temp == NULL) {
            fputs("Error: realloc shrink fail\n", stderr);
            return -1;
        }

        plist->fds = temp;
        plist->capacity -= 4;
    }

    plist->size--;

    return 0;
}

int poll_list_free(poll_list* plist) {
    free(plist->fds);
    plist->capacity = 0;
    plist->size = 0;

    return 0;
}