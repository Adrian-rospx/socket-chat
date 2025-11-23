#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

/* Formmated error logger, displaying text in RED */
void log_error(const char* fmt, ...);

/* Network error logger, accounting for operating system */
void log_network_error(const char* error_message);

#endif