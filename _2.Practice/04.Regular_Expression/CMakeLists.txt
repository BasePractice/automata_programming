cmake_minimum_required(VERSION 3.2.1)
project(04.Regular_Expression)


include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)

set(HEADER_FILES simple_regexp.h)
set(SOURCE_FILES simple_regexp.c)
set(MAIN_SOURCE_FILES main.c)
set(TESTS_SOURCE_FILES simple_regexp_tests.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${MAIN_SOURCE_FILES} ${HEADER_FILES})
add_executable(${PROJECT_NAME}_Tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} tests_main)
target_link_libraries(${PROJECT_NAME}_Tests tests_main)

