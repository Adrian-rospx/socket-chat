#ifndef POLL_LIST_H
#define POLL_LIST_H

#include "networking/os_networking.h"

/* Dynamic array of pollfd data types */
typedef struct {
    pollfd* fds;
    size_t size;
    size_t capacity;
} poll_list;

/* Initialise poll list */
int poll_list_init(poll_list* p_list);

/* Get the pollfd element at the specified fd in the poll list */
pollfd* poll_list_get(poll_list* plist, const socket_t fd);

/* Add a pollfd element to the list */
int poll_list_add(poll_list* p_list, const socket_t fd, const short events);

/*  Remove the element with the specified fd

    Throws fd not found and realloc errors 
*/
int poll_list_remove(poll_list* p_list, const socket_t fd);

/* Free the allocated memory */
int poll_list_free(poll_list* p_list);

#endif