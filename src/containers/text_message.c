
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "containers/test_message.h"

int text_message_init(text_message* msg, const uint8_t* text, const size_t length) {
    if (msg == NULL) {
        fputs("Error: message does not exit\n", stderr);
        return -1;
    }

    if (msg->buffer == NULL) {
        msg->buffer = malloc(length + 1 * sizeof(uint8_t));

        if (msg->buffer == NULL) {
            fputs("Error: couldn't allocate message memory\n", stderr);
            return -1;
        }
    } else {
        uint8_t* temp_ptr = realloc(msg->buffer, length + 1);

        if (temp_ptr == NULL) {
            fputs("Error: couldn't reallocate message memory\n", stderr);
            return -1;
        }
        
        msg->buffer = temp_ptr;
    }

    msg->capacity = length;
    msg->length = length;
    
    memcpy(msg, text, length);

    return 0;
}


int text_message_copy(text_message* msg1, text_message* msg2) {
    if (msg2 == NULL) {
        fputs("Error: message 2 does not exit\n", stderr);
        return -1;
    }
    return text_message_init(msg1, msg2->buffer, msg2->length);
}

int text_message_free(text_message* msg) {
    free(msg->buffer);
    msg->capacity = 0;
    msg->length = 0;

    return 0;
}