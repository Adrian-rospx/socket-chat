#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "client.h"

typedef struct app_flags {
    char app_mode;  // 'c' - client, 's' - server
    unsigned short port;
    char ip_address[256];
} app_flags;

int main(int argc, char** argv) {
    app_flags flags = {0};

    // default values
    strcpy(flags.ip_address, "127.0.0.1");
    flags.port = 8765;

    // handle flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 || 
            strcmp(argv[i], "-s") == 0) {
            // server mode
            if (flags.app_mode != 0) {
                fputs("Error: app mode can only be --server or --client", stderr);
                return -1;
            }
            flags.app_mode = 's';
        } else if (strcmp(argv[i], "--client") == 0 || 
            strcmp(argv[i], "-c") == 0) {
            // client mode
            if (flags.app_mode != 0) {
                fputs("Error: App mode can only be --server or --client", stderr);
                return -1;
            }
            flags.app_mode = 'c';
        } else if (strcmp(argv[i], "--port") == 0 || 
            strcmp(argv[i], "-p") == 0) {
            // set port number
            if (i + 1 >= argc) {
                fputs("Error: port value missing", stderr);
                return -1;
            }

            char *endptr;
            long port_value = strtol(argv[i+1], &endptr, 10);
        
            if (*endptr != '\0') {
                fputs("Error: invalid port value\n", stderr);
                return -1;
            }
            if (port_value < 0 || port_value > 65535) {
                fputs("Error: port value out of range\n", stderr);
                return -1;
            }

            flags.port = (unsigned short)port_value;
            i++;
        } else if (strcmp(argv[i], "--address") == 0 || 
            strcmp(argv[i], "-a") == 0) {
            // set server ip address
            if (i + 1 >= argc) {
                fputs("Error: --address value missing\n", stderr);
                return -1;
            }

            snprintf(flags.ip_address, sizeof(flags.ip_address), "%s", argv[i + 1]);
            i++;
        } else {
            fprintf(stderr, "Error: Invalid flag at index %d\n", i);
            return -1;
        }
    }

    switch (flags.app_mode) {
        case 's':
            run_server(flags.port);
            break;
        case 'c':
            run_client(flags.port, flags.ip_address);
            break;
        default:
            fputs("Error: Must choose either --server or --client", stderr);
            return -1;
    }

    fputs("Process ended successfully.\n", stdout);
    
    return 0;
}