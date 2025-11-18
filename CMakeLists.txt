cmake_minimum_required(VERSION 3.23)

set(CMAKE_C_STANDARD 17)

project(
    SocketChat 
    VERSION 0.2
    LANGUAGES C)

add_executable(SocketChat)

# source code files
target_sources(SocketChat
    PRIVATE 
        src/main.c
        src/server.c
        src/client.c
        src/socket_commands.c
        src/containers/poll_list.c
        src/containers/socket_buffer.c
        src/containers/sockbuf_list.c
)

# headers
target_include_directories(SocketChat PRIVATE include)

target_compile_options(SocketChat PRIVATE
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
    -ffile-prefix-map=${CMAKE_BINARY_DIR}=.
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # debug errors
    target_compile_options(SocketChat PRIVATE 
        -Wall 
        -Wextra
        -pedantic
    )
    # sanitizer
    target_compile_options(SocketChat PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
    )
    target_link_options(SocketChat PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
    )
endif()