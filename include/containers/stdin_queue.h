#ifndef STDIN_QUEUE_H
#define STDIN_QUEUE_H

#include "containers/text_message.h"
#include <stddef.h>
#include <threads.h>

/* Queue for managing incoming messages from the stdin thread */
typedef struct {
    text_message* messages;
    size_t size;
    size_t capacity;
    mtx_t lock;
} stdin_queue;

/* Initialise the queue */
int stdin_queue_init(stdin_queue* queue);

/* Checks if the queue has messages */
int stdin_queue_is_empty(stdin_queue* queue);

/* Copies the new message into the queue */
int stdin_queue_push(stdin_queue* queue, text_message* message);

/*  Copies the first element into the new message 
    and removes it from the queue
*/
int stdin_queue_pop(stdin_queue* queue, text_message* new_message);

#endif