#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "client.h"

typedef struct app_flags {
    char app_mode;  // 'c' - client, 's' - server
    unsigned short port;
    char ip_address[128];
} app_flags;

int handle_flags(const int argc, char** argv, 
    app_flags* flags, 
    const unsigned short default_port,
    const char* default_address) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 || 
            strcmp(argv[i], "-s") == 0) {
            // server mode
            if (flags->app_mode != 0) {
                fputs("Error: app mode can only be --server or --client\n", stderr);
                return EXIT_FAILURE;
            }

            flags->app_mode = 's';

            // set defaults
            flags->port = default_port;
            strcpy(flags->ip_address, default_address);
        } else if (strcmp(argv[i], "--client") == 0 || 
            strcmp(argv[i], "-c") == 0) {
            // client mode
            if (flags->app_mode != 0) {
                fputs("Error: App mode can only be --server or --client\n", stderr);
                return EXIT_FAILURE;
            }

            flags->app_mode = 'c';
            
            // set defaults
            flags->port = default_port;
            strcpy(flags->ip_address, default_address);
        } else if (strcmp(argv[i], "--port") == 0 || 
            strcmp(argv[i], "-p") == 0) {
            // set port number
            if (i + 1 >= argc) {
                fputs("Error: port value missing\n", stderr);
                return EXIT_FAILURE;
            }

            char *endptr;
            long port_value = strtol(argv[i+1], &endptr, 10);
        
            if (*endptr != '\0') {
                fputs("Error: invalid port value\n", stderr);
                return EXIT_FAILURE;
            }
            if (port_value < 0 || port_value > 65535) {
                fputs("Error: port value out of range\n", stderr);
                return EXIT_FAILURE;
            }

            flags->port = (unsigned short)port_value;
            i++;
        } else if (strcmp(argv[i], "--address") == 0 || 
            strcmp(argv[i], "-a") == 0) {
            // set server ip address
            if (i + 1 >= argc) {
                fputs("Error: --address value missing\n", stderr);
                return EXIT_FAILURE;
            }

            snprintf(flags->ip_address, sizeof(flags->ip_address), 
                "%s", argv[i + 1]);
            i++;
        } else {
            fprintf(stderr, "Error: Invalid flag at index %d\n", i);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int handle_input_options(app_flags* flags, 
    const unsigned short default_port,
    const char* default_address) {

    char buffer[128] = {0};

    // input app_mode
    while (flags->app_mode == '\0') {
        fputs("Run (server, s or client, c): ", 
            stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            fputs("Error: failed to read input\n", stderr);

            return EXIT_FAILURE;
        }

        // clean input buffer
        // int c;
        // while ((c = getchar()) != '\n' && c != EOF) {};

        // strip trailing newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (strcmp(buffer, "s") == 0 || 
            strcmp(buffer, "server") == 0) {
            flags->app_mode = 's';
            break;
        } else if (strcmp(buffer, "c") == 0 || 
            strcmp(buffer, "client") == 0) {
            flags->app_mode = 'c';
            break;
        } else {
            fprintf(stderr, "Error: Invalid option \"%s\"\n", buffer);
            continue;
        }
    }

    // input port
    while (flags->port == 0) {
        fputs("Enter port number [0 - 65535] default 8765: ", 
            stdout);
            
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            fputs("Error: failed to read input\n", stderr);
            return EXIT_FAILURE;
        }

        // clean input buffer
        // int c;
        // while ((c = getchar()) != '\n' && c != EOF) {};

        // default value
        if (*buffer == '\n') {
            flags->port = default_port;
            break;
        }

        char* endptr;
        long port_value = strtol(buffer, &endptr, 10);

        if (*endptr == '\0') {
            fputs("Error: invalid port value\n", stderr);
            continue;
        }
        if (port_value <= 0 || port_value > 65535) {
            fputs("Error: port value out of range\n", stderr);
            continue;
        }

        flags->port = (unsigned short)port_value;
    }
    // input ip address
    while (*flags->ip_address == '\0' && flags->app_mode == 'c') {
        fputs("Enter server IP address [x.x.x.x] default 127.0.0.1: ", stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            fputs("Error: failed to read input\n", stderr);
            return EXIT_FAILURE;
        }

        // clean input buffer
        // int c;
        // while ((c = getchar()) != '\n' && c != EOF) {};

        // strip trailing newline
        buffer[strcspn(buffer, "\n")] = '\0';
        
        if (*buffer == '\0') {
            strcpy(flags->ip_address, default_address);
            break;
        }

        snprintf(flags->ip_address, sizeof(flags->ip_address), 
            "%s", buffer);
    }
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    app_flags flags = {0};

    const unsigned short default_port = 8765;
    const char default_address[32] = "127.0.0.1";

    // handle flags
    if (handle_flags(argc, argv, &flags, default_port, default_address) == EXIT_FAILURE)
        return EXIT_FAILURE;
    
    // call user input
    if (handle_input_options(&flags, default_port, default_address) == EXIT_FAILURE)
        return EXIT_FAILURE;

    // execute
    switch (flags.app_mode) {
        case 's':
            run_server(flags.port);
            break;
        case 'c':
            run_client(flags.port, flags.ip_address);
            break;
        default:
            fputs("Error: Must choose either --server or --client\n", stderr);
            return EXIT_FAILURE;
    }

    fputs("Process ended.\n", stdout);
    
    return 0;
}