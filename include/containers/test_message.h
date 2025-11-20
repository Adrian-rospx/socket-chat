#ifndef TEXT_MESSAGE_H
#define TEXT_MESSAGE_H

#include <stddef.h>

/* Text message container */
typedef struct {
    char* buffer;
    size_t length;
    size_t capacity;
} text_message;

int text_message_init(text_message* msg, char* text, size_t length);

int text_message_copy(text_message* msg1, text_message* msg2);

int text_message_free(text_message* msg);

#endif