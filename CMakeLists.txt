cmake_minimum_required(VERSION 3.23)

set(CMAKE_C_STANDARD 17)

project(
    SocketChat 
    VERSION 0.3
    LANGUAGES C
)

add_executable(SocketChat)

# source code files
target_sources(SocketChat
    PRIVATE 
        src/main.c
        src/server.c
        src/client.c
        src/containers/poll_list.c
        src/containers/socket_buffer.c
        src/containers/sockbuf_list.c
        src/containers/text_message.c
        src/utils/socket_commands.c
        src/utils/logging.c
        src/events/data_pipes.c
        src/events/client_events.c
        src/events/server_events.c
)
target_include_directories(SocketChat PRIVATE include)

target_compile_options(SocketChat PRIVATE
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
    -ffile-prefix-map=${CMAKE_BINARY_DIR}=.
)

if (WIN32)
    target_link_libraries(SocketChat PRIVATE ws2_32)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # debug errors
    target_compile_options(SocketChat PRIVATE 
        -Wall 
        -Wextra
        -pedantic
    )
    # sanitizer
    # for windows: clang requires the asan dll libraries! 
    target_compile_options(SocketChat PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(SocketChat PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

endif()