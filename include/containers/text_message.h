#ifndef TEXT_MESSAGE_H
#define TEXT_MESSAGE_H

#include <stddef.h>
#include <stdint.h>

/* Text message dynamic container */
typedef struct {
    uint8_t* buffer;
    size_t length;
    size_t capacity;
} text_message;

/* Initialise the text message container with a default size */
int text_message_init(text_message* msg);

/* Insert the message into the container */
int text_message_create(text_message* msg, const uint8_t* text, size_t length);

/* Copy a message into the new container */
int text_message_copy(text_message* msg_old, text_message* msg_new);

int text_message_free(text_message* msg);

#endif