#ifndef LOGGING_H
#define LOGGING_H

#include <stdarg.h>

void log_error(const char* fmt, ...);

void log_network_error(const char* error_message);

#endif