#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "containers/text_message.h"
#include "containers/thread_queue.h"
#include "utils/logging.h"

#include "utils/stdin_thread.h"

#define STDIN_BUFFER_SIZE 1024

int stdin_thread_function(void* arg) {
    stdin_thread_args* args = arg;

    // obtain queue from arg
    thread_queue* queue = args->queue;

    char buffer[STDIN_BUFFER_SIZE];

    while (1) {
        if (fgets(buffer, STDIN_BUFFER_SIZE, stdin) == NULL)
            break;

        // strip newline
        size_t len = strlen(buffer);
        if (len && buffer[len-1] == '\n') 
            buffer[len-1] = '\0';

        log_extra_info("Stdin message: %s\n", buffer);
        len--;

        text_message msg = {0};
        text_message_init(&msg);

        if (text_message_create(&msg, (uint8_t*)buffer, len-1) == EXIT_FAILURE)
            continue;

        if (thread_queue_push(queue, &msg) == EXIT_FAILURE)
            continue;
        
        text_message_free(&msg);
    }

    return EXIT_SUCCESS;
}