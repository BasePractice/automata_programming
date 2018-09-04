cmake_minimum_required(VERSION 3.2.1)
project(01.Turing_Machine)


include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

set(HEADER_FILES turing_machine.h)
set(SOURCE_FILES turing_machine.c)
set(MAIN_SOURCE_FILES main.c)
set(TESTS_SOURCE_FILES turing_machine_test.cpp)

add_executable(${CURRENT_PROJECT_NAME} ${SOURCE_FILES} ${MAIN_SOURCE_FILES} ${HEADER_FILES})
add_executable(${CURRENT_PROJECT_NAME}_Tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})
