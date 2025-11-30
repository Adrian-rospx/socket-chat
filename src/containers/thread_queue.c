#include "containers/thread_queue.h"
#include "containers/text_message.h"
#include "utils/logging.h"
#include <float.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#define DEF_QUEUE_SIZE 16

int thread_queue_init(thread_queue* queue) {
    queue->messages = malloc(sizeof(text_message) * DEF_QUEUE_SIZE);
    if (queue->messages == NULL) {
        log_error("Could allocate thread queue memory");
        return EXIT_FAILURE;
    }
    
    queue->capacity = DEF_QUEUE_SIZE;
    queue->head = 0;
    queue->tail = 0;
    
    mtx_init(&queue->lock, mtx_plain);
    return EXIT_SUCCESS;
}

int thread_queue_is_empty(thread_queue* queue) {
    return queue->head == queue->tail;
}

int thread_queue_push(thread_queue* queue, text_message* message_pushed) {
    mtx_lock(&queue->lock);

    size_t next_index = (queue->tail + 1) % queue->capacity;
    
    if (next_index == queue->head) {
        // queue full
        mtx_unlock(&queue->lock);
        return EXIT_FAILURE;
    }

    // copy pushed item to thread queue
    text_message_init(&queue->messages[queue->tail]);
    
    if (text_message_copy(
                &queue->messages[queue->tail],
                message_pushed) == EXIT_FAILURE) {
        // copy failed
        mtx_unlock(&queue->lock);
        return EXIT_FAILURE;
    }

    queue->tail = next_index;

    mtx_unlock(&queue->lock);
    return EXIT_SUCCESS;
}

text_message* thread_queue_pop(thread_queue* queue) {
    mtx_lock(&queue->lock);

    if (thread_queue_is_empty(queue)) {
        mtx_unlock(&queue->lock);
        
        return NULL;
    }

    text_message* ref = &queue->messages[queue->head];

    text_message* msg = malloc(sizeof(text_message));
    if (msg == NULL) {
        log_error("Couldn't allocate queue popped message memory");
        return NULL;
    }

    if (text_message_init(msg) == EXIT_FAILURE) {
        free(msg);
        return NULL;
    }
    
    if (text_message_copy(msg, ref) == EXIT_FAILURE) {
        text_message_free(msg);
        free(msg);
        return NULL;
    }

    queue->head = (queue->head + 1) % queue->capacity;
    text_message_free(ref);

    mtx_unlock(&queue->lock);
    return msg;
}

int thread_queue_free(thread_queue* queue) {
    for (size_t i = queue->head; i != queue->tail; i = (i+1) % queue->capacity) {
        text_message_free(&queue->messages[i]);
    }
    
    free(queue->messages);
    queue->messages = NULL;

    queue->capacity = 0;
    queue->head = 0;
    queue->tail = 0;

    return EXIT_SUCCESS;
}
