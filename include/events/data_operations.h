#ifndef DATA_OPERATIONS_H
#define DATA_OPERATIONS_H

#include "containers/socket_buffer.h"

/*  Process incoming buffer data to send complete 
    messages to the outgoing buffer
*/
int pipe_incoming_to_outgoing(socket_buffer* s_buf);

#endif