#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H

#include "os_networking.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"

/* Handle new client connections */
int server_connect_event(poll_list* p_list, sockbuf_list* sbuf_list);

/*  Read from client into the incoming buffer

    Sets POLLOUT flag when done
*/
int server_read_event(poll_list* p_list, sockbuf_list* sbuf_list, const socket_t fd);

/*  Write outgoing buffer contents 

    Resets POLLOUT flag when done
*/
int server_write_event(sockbuf_list* sbuf_list, poll_list* p_list, const socket_t fd);

#endif