#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"

/*  Handle new client connections

    Returns EXIT_FAILURE on accept fail
*/
int server_connect_event_handler(poll_list* p_list, sockbuf_list* sbuf_list);

#endif