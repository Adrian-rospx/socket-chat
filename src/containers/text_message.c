
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "containers/text_message.h"
#include "utils/logging.h"

#define DEF_MSG_ALLOC 64

int text_message_init(text_message* msg) {
    msg->buffer = malloc(DEF_MSG_ALLOC);

    if (msg->buffer == NULL) {
        log_error("Couldn't allocate message memory");
        return EXIT_FAILURE;
    }

    msg->capacity = DEF_MSG_ALLOC;
    msg->length = 0;

    return EXIT_SUCCESS;
}

int text_message_create(text_message* msg, const uint8_t* text, const size_t length) {
    if (msg == NULL) {
        log_error("Message does not exist");
        return EXIT_FAILURE;
    }

    if (msg->buffer == NULL && length < DEF_MSG_ALLOC) {
        if (text_message_init(msg) == EXIT_FAILURE)
            return EXIT_FAILURE;
    } else {
        uint8_t* temp_ptr = realloc(msg->buffer, length);

        if (temp_ptr == NULL) {
            log_error("Couldn't reallocate message memory");
            return EXIT_FAILURE;
        }
        
        msg->buffer = temp_ptr;
        msg->capacity = length;
    }

    msg->length = length;
    
    memcpy(msg->buffer, text, length);

    return EXIT_SUCCESS;
}

int text_message_copy(text_message* msg_old, text_message* msg_new) {
    if (msg_new == NULL) {
        log_error("New message to copy does not exist");
        return EXIT_FAILURE;
    }
    return text_message_create(msg_old, msg_new->buffer, msg_new->length);
}

int text_message_free(text_message* msg) {
    free(msg->buffer);
    msg->buffer = NULL;

    msg->capacity = 0;
    msg->length = 0;

    return EXIT_SUCCESS;
}