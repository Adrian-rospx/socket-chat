#ifndef STDIN_THREAD_H
#define STDIN_THREAD_H

#include "containers/thread_queue.h"

typedef struct {
    thread_queue* queue;
} stdin_thread_args;

/*  Run a thread that takes blocking input from stdin
   
    and adds it to the thread message queue
*/
int stdin_thread_function(void* arg);

#endif