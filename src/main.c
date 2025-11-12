#include <stdio.h>
#include <string.h>

#include "include/server.h"
#include "include/client.h"

int input_error_instruction() {
    fputs("Invalid input!\n", stderr);
    fputs("Usage: enter -c for client or -s for server\n", 
        stderr
    );
    return 1;
}

int main(int argc, char** argv) {
    if (argc < 2)
        return input_error_instruction();

    if (strcmp(argv[1], "-s") == 0) {
        if (run_server() == -1) {
            fputs("Server error", stderr);
            return 1;
        }
    }
    else if (strcmp(argv[1], "-c") == 0) {
        if (run_client() == -1) {
            fputs("Client error", stderr);
            return 1;
        }
    }
    else
        return input_error_instruction();

    fputs("Process ended successfully.\n", stdout);
    
    return 0;
}