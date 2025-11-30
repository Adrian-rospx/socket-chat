#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include "containers/text_message.h"
#include <stddef.h>
#include <threads.h>

/* Queue for managing incoming messages from the stdin thread */
typedef struct {
    text_message* messages;
    size_t head;
    size_t tail;
    size_t capacity;
    mtx_t lock;
} thread_queue;

/* Initialise the queue */
int thread_queue_init(thread_queue* queue);

/* Checks if the queue has messages (1-true, 0-false) */
int thread_queue_is_empty(thread_queue* queue);

/* Copies the new message into the queue */
int thread_queue_push(thread_queue* queue, text_message* message_pushed);

/* Return a dynamically allocated copy of the first queued element */
text_message* thread_queue_pop(thread_queue* queue);

/* Free queue and all allocated contents */
int thread_queue_free(thread_queue* queue);

#endif