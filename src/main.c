#include <stdio.h>
#include <string.h>

#include "include/server.h"
#include "include/client.h"

int input_error_instruction() {
    fputs("Invalid input!\n", stdout);
    fputs("Usage: enter -c for client or -s for server\n", 
        stdout
    );
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return input_error_instruction();
    }

    if (strcmp(argv[1], "-s") == 0)
        fputs("Server mode!\n", stdout);
    else if (strcmp(argv[1], "-c") == 0)
        fputs("Client mode!\n", stdout);
    else {
        return input_error_instruction();
    }
    // run_server();

    fputs("Process ended.\n", stdout);
    
    return 0;
}