cmake_minimum_required(VERSION 3.26)
project(assignment2 C)

set(CMAKE_C_STANDARD 17)

add_executable(client client.c)  # Include any other source files
add_executable(server server.c)
