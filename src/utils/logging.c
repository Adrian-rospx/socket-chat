#include <stdio.h>
#include <stdarg.h>
#include <vadefs.h>

#include "os_networking.h"

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    va_end(args);
}

void log_network_error(const char* error_message) {
#ifdef _WIN32
    fprintf(stderr, "%s WSA Error code: %d\n", 
        error_message, WSAGetLastError());
#else
    perror(error_message);
#endif
} 