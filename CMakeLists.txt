cmake_minimum_required(VERSION 3.23)

set(CMAKE_C_STANDARD 17)

project(
    socket-chat 
    VERSION 0.4
    LANGUAGES C
)

add_executable(socketchat)

# source code files
target_sources(socketchat
    PRIVATE 
        src/main.c
        src/server.c
        src/client.c
        
        src/containers/poll_list.c
        src/containers/socket_buffer.c
        src/containers/sockbuf_list.c
        src/containers/text_message.c
        src/containers/thread_queue.c

        src/networking/socket_commands.c
        src/networking/data_pipes.c
        
        src/events/server_events.c
        
        src/utils/logging.c
        src/utils/stdin_thread.c
)
target_include_directories(socketchat PRIVATE include)

target_compile_options(socketchat PRIVATE
    -ffile-prefix-map=${CMAKE_SOURCE_DIR}=.
    -ffile-prefix-map=${CMAKE_BINARY_DIR}=.
)

if (WIN32)
    target_link_libraries(socketchat PRIVATE ws2_32)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    # debug errors
    target_compile_options(socketchat PRIVATE 
        -Wall 
        -Wextra
        -pedantic
    )
    # sanitizer
    # for windows: clang requires the asan dll libraries! 
    target_compile_options(socketchat PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(socketchat PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

endif()