#ifndef DATA_OPERATIONS_H
#define DATA_OPERATIONS_H

#include "containers/sockbuf_list.h"
#include "containers/socket_buffer.h"
#include "containers/text_message.h"
#include "containers/poll_list.h"

#include "networking/os_networking.h"

/*  Process incoming buffer data to create
    complete messages
*/
int pipe_incoming_to_message(socket_buffer* sock_buf, text_message* txt_msg);

/*  Write message to screen */

int pipe_message_to_stdout(text_message* txt_msg);

/*  Send the message to the outgoing buffer of the specified fd

    Activates the POLLOUT flag 
*/
int pipe_message_to_outgoing(sockbuf_list* sbuf_list, poll_list* p_list, 
    socket_t fd, text_message* txt_msg);

/*  Sends the message to all socket buffers,
    effecitvely broadcasting it to all users.
*/
int pipe_message_to_all(sockbuf_list* sbuf_list, poll_list* p_list, 
    socket_t fd, text_message* txt_msg);

#endif