#include <memory.h>
#include <stdlib.h>

#include "containers/socket_buffer.h"

int pipe_incoming_to_outgoing(socket_buffer* s_buf) {
    // process new data
    if (!s_buf->has_length) {
        if (s_buf->incoming_length < 4)
            return 0; // not enough data

        // register new message length
        uint32_t raw_prefix;
        memcpy(&raw_prefix, s_buf->incoming_buffer, sizeof(uint32_t));

        const uint32_t prefix = ntohl(raw_prefix);
        fprintf(stdout, "Expected message length: %d\n", prefix);

        s_buf->exp_msg_len = prefix;
        s_buf->has_length = 1;

        // remove it from the buffer
        socket_buffer_deque_incoming(s_buf, sizeof(uint32_t));
    }

    if (s_buf->has_length && s_buf->incoming_length >= s_buf->exp_msg_len) {
        // process message
        uint8_t* msg = malloc((s_buf->exp_msg_len + 1) * sizeof(uint8_t));
        if (msg == NULL) {
            fputs("Error: couldn't allocate message memory", stderr);
            return -1;
        }

        strncpy((char*)msg, (char*)s_buf->incoming_buffer, s_buf->exp_msg_len);
        msg[s_buf->exp_msg_len] = '\0';

        fprintf(stdout, "Message passing: %s\n", msg);
        
        // queue to output
        socket_buffer_queue_outgoing(s_buf, msg, s_buf->exp_msg_len);

        free(msg);

        // remove message from buffer
        socket_buffer_deque_incoming(s_buf, s_buf->exp_msg_len);
        s_buf->has_length = 0;

        return 0;
    }

    return 0; // incomplete data
}
