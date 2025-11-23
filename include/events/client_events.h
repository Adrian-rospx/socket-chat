#ifndef CLIENT_EVENTS_H
#define CLIENT_EVENTS_H

#include "containers/sockbuf_list.h"
#include "containers/socket_buffer.h"

/* Read message from standard input and write to incoming buffer */
int client_stdin_event(sockbuf_list* sbuf_list, poll_list* p_list, socket_t fd);

/* Send message from outgoing buffer to server */
int client_write_event(socket_buffer* sock_buf, poll_list* p_list);

/* Read message from server and write to incoming buffer */
int client_read_event(socket_buffer* sock_buf);

#endif