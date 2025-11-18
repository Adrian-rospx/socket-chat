#ifndef OS_NETWORKING_H
#define OS_NETWORKING_H

// system header includes
#ifdef _WIN32

#else
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <sys/poll.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
#endif

// socket type abstraction
#ifdef _WIN32
    typedef SOCKET socket_t;
    #define SOCKET_INVALID INVALID_SOCKET
#else
    typedef int socket_t;
    #define SOCKET_INVALID -1
#endif

#endif // all