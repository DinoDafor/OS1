cmake_minimum_required(VERSION 3.17)
project(LabOS1 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
add_executable(LabOS1 main.c)