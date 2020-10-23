cmake_minimum_required(VERSION 3.2.1)
project(10.FSM_InsertElement)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)

set(HEADER_FILES inserting_fsm.h)
set(SOURCE_FILES inserting_fsm.c)
set(TESTS_SOURCE_FILES inserting_tests.cpp)

add_executable(${PROJECT_NAME}_Main inserting_main.c ${SOURCE_FILES})
add_executable(${PROJECT_NAME}_Tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME}_Tests tests_main)


