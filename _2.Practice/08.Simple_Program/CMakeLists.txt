cmake_minimum_required(VERSION 3.2.1)
project(08.Simple_Program)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)


add_executable(${PROJECT_NAME} simple_main.c)


