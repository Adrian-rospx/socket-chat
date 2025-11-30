#include <stdio.h>
#include <stdarg.h>
#include <vadefs.h>

#include "networking/os_networking.h"

#define ANSI_BLACK          "\e[30m"
#define ANSI_RED            "\e[31m"
#define ANSI_YELLOW         "\e[33m"
#define ANSI_CYAN           "\e[36m"
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

void log_warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, ANSI_YELLOW "Warning: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, ANSI_RESET_COLOR "\n");

    va_end(args);
}

void log_extra_info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    fprintf(stdout, ANSI_BLACK "[info]: ");
    vfprintf(stdout, fmt, args);
    fprintf(stdout, ANSI_RESET_COLOR "\n");

    va_end(args);
}

void log_event(const char* msg) {
    fprintf(stdout, 
        ANSI_CYAN "[event]: %s\n" ANSI_RESET_COLOR, msg);
}
