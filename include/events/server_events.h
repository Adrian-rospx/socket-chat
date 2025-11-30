#ifndef SERVER_EVENTS_H
#define SERVER_EVENTS_H

#include "containers/text_message.h"
#include "networking/os_networking.h"

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"

/* Handle new client connections */
int server_connect_event_handler(poll_list* p_list, sockbuf_list* sbuf_list);

/*  Read from client into the incoming buffer. Exit codes:

    EXIT_SUCCESS - message message returned completely

    EXIT_FAILURE - error or disconnect

    2 - incomplete message
*/
int read_event_handler(poll_list* p_list, sockbuf_list* sbuf_list, 
            text_message* msg, const socket_t fd);

/*  Write outgoing buffer contents 

    Resets POLLOUT flag when done
*/
int write_event_handler(sockbuf_list* sbuf_list, poll_list* p_list, 
            const socket_t fd);

#endif