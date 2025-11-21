#ifndef CLIENT_EVENTS_H
#define CLIENT_EVENTS_H

#include "containers/socket_buffer.h"

/* Read message from standard input and write to incoming buffer */
int client_stdin_event(socket_buffer* sock_buf, poll_list* p_list);

/* Send message from outgoing buffer to server */
int client_write_event(socket_buffer* sock_buf, poll_list* p_list);

/* Read message from server and write to incoming buffer */
int client_read_event(socket_buffer* sock_buf);

#endif