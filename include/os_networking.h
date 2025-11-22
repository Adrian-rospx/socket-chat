#ifndef OS_NETWORKING_H
#define OS_NETWORKING_H

// system header includes
#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
    #include <WinSock2.h>
    #include <WS2tcpip.h> // requires the ws2_32.lib library
    #include <basetsd.h>
    #include <minwindef.h>
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

// socket closing function
static inline void socket_close(socket_t fd) {
    #ifdef _WIN32
        closesocket(fd);
    #else
        close(fd);
    #endif
}

// wrap nonblocking mode options
static inline int socket_set_nonblocking(socket_t fd) {
    #ifdef _WIN32
        u_long mode = 1;
        return ioctlsocket(fd, FIONBIO, &mode);
    #else
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1) {
            perror("fcntl F_GETFL");
            return SOCKET_INVALID;
        }
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
            perror("fcntl F_SETFL");
            return SOCKET_INVALID;
        }
        return fd;
    #endif
}

// socket init/cleanup wrapper for windows
static inline int winsock_init(void) {
    #ifdef _WIN32
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
    #endif
    
    return 0;
}
static inline int winsock_cleanup(void) {
    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}

// replace poll with alias
#ifdef _WIN32
    typedef WSAPOLLFD pollfd;
    #define poll WSAPoll
#else
    typedef struct pollfd pollfd;
#endif

// define unix types
#ifdef _WIN32
    typedef SSIZE_T ssize_t;
#endif

// unix console input
#ifdef _WIN32
    #include <io.h>
    #define read _read

    #define STDIN_FILENO _fileno(stdin) 
#endif

#endif // end of header