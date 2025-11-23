
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "containers/text_message.h"
#include "utils/logging.h"

int text_message_init(text_message* msg, const uint8_t* text, const size_t length) {
    if (msg == NULL) {
        log_error("Message does not exist");
        return EXIT_FAILURE;
    }

    if (msg->buffer == NULL) {
        msg->buffer = malloc(length * sizeof(uint8_t));

        if (msg->buffer == NULL) {
            log_error("Couldn't allocate message memory");
            return EXIT_FAILURE;
        }
    } else {
        uint8_t* temp_ptr = realloc(msg->buffer, length);

        if (temp_ptr == NULL) {
            log_error("Couldn't reallocate message memory");
            return EXIT_FAILURE;
        }
        
        msg->buffer = temp_ptr;
    }

    msg->capacity = length;
    msg->length = length;
    
    memcpy(msg->buffer, text, length);

    return EXIT_SUCCESS;
}

int text_message_copy(text_message* msg1, text_message* msg2) {
    if (msg2 == NULL) {
        log_error("Message 2 does not exist");
        return EXIT_FAILURE;
    }
    return text_message_init(msg1, msg2->buffer, msg2->length);
}

int text_message_free(text_message* msg) {
    free(msg->buffer);
    msg->buffer = NULL;

    msg->capacity = 0;
    msg->length = 0;

    return EXIT_SUCCESS;
}