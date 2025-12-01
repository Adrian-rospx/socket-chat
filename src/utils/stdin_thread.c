#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "containers/text_message.h"
#include "containers/thread_queue.h"
#include "networking/os_networking.h"

#include "utils/stdin_thread.h"

#define STDIN_BUFFER_SIZE 1024

int stdin_thread_function(void* arg) {
    stdin_thread_args* args = arg;

    // obtain queue from arg
    thread_queue* queue = args->queue;
    socket_t notify_send_fd = args->notify_send_fd;

    char buffer[STDIN_BUFFER_SIZE];

    while (1) {
        if (fgets(buffer, STDIN_BUFFER_SIZE-1, stdin) == NULL)
            break;

        buffer[strcspn(buffer, "\n\r")] = 0;

        // strip newline
        size_t len = strlen(buffer);

        if (len == 0) {
            continue;
        }

        text_message msg = {0};
        if (text_message_init(&msg) == EXIT_FAILURE) {
            continue;
        }

        if (text_message_create(&msg, (uint8_t*)buffer, len) == EXIT_FAILURE) {
            text_message_free(&msg);
            continue;
        }

        if (thread_queue_push(queue, &msg) == EXIT_FAILURE) {
            text_message_free(&msg);
            continue;
        }

        // notify main thread
        send(notify_send_fd, "x", 1, 0);
        
        text_message_free(&msg);
    }

    return EXIT_SUCCESS;
}