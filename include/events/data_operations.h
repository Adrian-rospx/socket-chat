#ifndef DATA_OPERATIONS_H
#define DATA_OPERATIONS_H

#include "containers/socket_buffer.h"

/*  Process incoming buffer data to send complete 
    messages to the outgoing buffer

    Sets POLLOUT flag when finished
*/
int pipe_incoming_to_outgoing(socket_buffer* s_buf, poll_list* p_list);

#endif