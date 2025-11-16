cmake_minimum_required(VERSION 3.23)

project(SocketChat LANGUAGES C)

set(CMAKE_C_STANDARD 17)

add_executable(SocketChat)

target_sources(SocketChat
    PRIVATE 
        src/main.c
        src/server.c
        src/client.c
        src/network.c
        src/poll_list.c
)

target_include_directories(SocketChat PRIVATE include)

target_compile_options(SocketChat 
    PRIVATE 
        -Wall 
        -Wextra
        -pedantic
)
