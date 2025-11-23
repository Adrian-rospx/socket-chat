#include <stdio.h>
#include <stdarg.h>

#include "os_networking.h"

#define ANSI_RED            "\e[31m"
#define ANSI_RESET_COLOR    "\e[0m"

void log_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, ANSI_RED "Error: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, ANSI_RESET_COLOR "\n");

    va_end(args);
}

void log_network_error(const char* error_message) {
#ifdef _WIN32
    fprintf(stderr, ANSI_RED "%s WSA Error code: %d\n" ANSI_RESET_COLOR, 
        error_message, WSAGetLastError());
#else
    perror(error_message);
#endif
} 