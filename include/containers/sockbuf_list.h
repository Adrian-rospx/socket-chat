#ifndef SOCKBUF_LIST_H
#define SOCKBUF_LIST_H

#include "os_networking.h"
#include "containers/socket_buffer.h"

/* Dynamic array (list) of socket buffer types */
typedef struct {
    socket_buffer* bufs;
    size_t size;
    size_t capacity;
} sockbuf_list;

/*  Initialise socket buffer memory 

    Throws malloc errors
*/
int sockbuf_list_init(sockbuf_list* sbuf_l);

socket_buffer* sockbuf_list_get(const sockbuf_list* sbuf_l, const socket_t fd);

/*  Add a new socket buffer to the the list 

    Throws memory realloc errors
*/
int sockbuf_list_append(sockbuf_list* sbuf_l, const socket_t fd);

/*  Remove the socket buffer at the fd from the list

    Throws fd not found and memory realloc errors
*/
int sockbuf_list_remove(sockbuf_list* sbuf_l, const socket_t fd);

/* Free list memory and all socket buffer memory */
int sockbuf_list_free(sockbuf_list* sbuf_l);

#endif