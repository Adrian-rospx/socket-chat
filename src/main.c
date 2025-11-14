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

void invalid_input(void) {
    fputs("Invalid input!\n", stderr);
    fputs("Usage: enter -c for client or -s for server\n", 
        stderr
    );
}

int main(int argc, char** argv) {
    app_flags flags = {0};

    // default values
    strcpy(flags.ip_address, "127.0.0.1");
    flags.port = 8765;


    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--server") == 0 || 
            strcmp(argv[i], "-s") == 0) {
            // server mode
            if (flags.app_mode != 0) {
                invalid_input();
                return -1;
            }
            flags.app_mode = 's';
        } else if (strcmp(argv[i], "--client") == 0 || 
            strcmp(argv[i], "-c") == 0) {
            // client mode
            if (flags.app_mode != 0) {
                invalid_input();
                return -1;
            }
            flags.app_mode = 'c';
        } else if (strcmp(argv[i], "--port") == 0 || 
            strcmp(argv[i], "-p") == 0) {
            // set port number
            if (i + 1 >= argc) {
                invalid_input();
                return -1;
            }

            char *endptr;
            long port_value = strtol(argv[i+1], &endptr, 10);
        
            if (*endptr != '\0') {
                invalid_input();
                return -1;
            }
            if (port_value < 0 || port_value > 65535) {
                invalid_input();
                return -1;
            }

            flags.port = (unsigned short)port_value;
            i++;
        } else if (strcmp(argv[i], "--address") == 0 || 
            strcmp(argv[i], "-a") == 0) {
            // set server ip address
            if (i + 1 >= argc) {
                invalid_input();
                return -1;
            }

            snprintf(flags.ip_address, sizeof(flags.ip_address), "%s", argv[i + 1]);
            i++;
        } else {
            invalid_input();
            return -1;
        }
    }

    fprintf(stdout, 
        "Current format:\napp mode: %c\nport: %hd\nIP address: %s\n", 
        flags.app_mode, flags.port, flags.ip_address);

    fputs("Process ended successfully.\n", stdout);
    
    return 0;
}