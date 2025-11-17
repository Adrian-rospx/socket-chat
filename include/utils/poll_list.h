#ifndef POLL_LIST_H
#define POLL_LIST_H

#include <sys/poll.h>
#include <sys/types.h>

typedef struct pollfd pollfd;

/* Dynamic array of pollfd data types */
typedef struct {
    pollfd* fds;
    size_t size;
    size_t capacity;
} poll_list;

/* Initialise poll list */
int poll_list_init(poll_list* p_list);

/* Add a pollfd element to the list */
int poll_list_add(poll_list* p_list, const int fd, const short events);

/*  Remove the element with the specified fd

    Throws fd not found and realloc errors 
*/
int poll_list_remove(poll_list* p_list, const int fd);

/* Free the allocated memory */
int poll_list_free(poll_list* p_list);

#endif