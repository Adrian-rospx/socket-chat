#ifndef STDIN_THREAD_H
#define STDIN_THREAD_H

#include "containers/thread_queue.h"
#include "networking/os_networking.h"

typedef struct {
    thread_queue* queue;
    socket_t notify_send_fd;
} stdin_thread_args;

/*  Run a thread that takes blocking input from stdin
   
    and adds it to the thread message queue
*/
int stdin_thread_function(void* arg);

#endif