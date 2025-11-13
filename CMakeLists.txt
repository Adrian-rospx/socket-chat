cmake_minimum_required(VERSION 3.23)

project(SocketChat LANGUAGES C)

add_executable(SocketChat)

target_sources(SocketChat
    PRIVATE 
        src/main.c
        src/server.c
        src/client.c
)
target_include_directories(SocketChat PRIVATE include)

target_compile_features(SocketChat PUBLIC c_std_17)
target_compile_options(SocketChat 
    PRIVATE 
        -Wall 
        -Wextra 
)
