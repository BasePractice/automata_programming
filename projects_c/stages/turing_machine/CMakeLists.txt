cmake_minimum_required(VERSION 3.4.1)
project(turing_machine)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.build")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

set(HEADER_FILES src/turing_machine.h)
set(SOURCE_FILES src/turing_machine.c)
set(MAIN_SOURCE_FILES src/main.c)
set(TESTS_SOURCE_FILES tests/turing_machine_test.cpp)

add_executable(turing_machine ${SOURCE_FILES} ${MAIN_SOURCE_FILES} ${HEADER_FILES})
add_executable(turing_machine_tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(turing_machine_tests gtest gtest_main)
