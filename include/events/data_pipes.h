#ifndef DATA_OPERATIONS_H
#define DATA_OPERATIONS_H

#include "containers/socket_buffer.h"
#include "containers/test_message.h"

/*  Process incoming buffer data to create
    complete messages
*/
int pipe_incoming_to_message(socket_buffer* sock_buf, text_message* txt_msg);

/*  Send the message to the outgoing buffer

    Activates the POLLOUT flag 
*/
int pipe_message_to_outgoing(socket_buffer* sock_buf, poll_list* p_list, text_message* txt_msg);

#endif