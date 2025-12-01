#ifndef EVENT_HANDLERS_H
#define EVENT_HANDLERS_H 

#include "containers/poll_list.h"
#include "containers/sockbuf_list.h"
#include "containers/text_message.h"
#include "containers/thread_queue.h"
#include "networking/os_networking.h"

typedef enum {
    EVENT_SOCKET_READ,
    EVENT_SOCKET_WRITE,
    EVENT_STDIN_READ,
} event_type;

typedef struct {
    event_type  type;
    void*       data;
} event;

typedef struct {
    socket_t        fd;
    text_message    msg;
} socket_event_data;

typedef struct {
    sockbuf_list   sbuf_l;
    poll_list      p_list;
    thread_queue   stdin_queue;
} client_loop_data;

int on_client_read(event* ev, void* connection_data);

#endif