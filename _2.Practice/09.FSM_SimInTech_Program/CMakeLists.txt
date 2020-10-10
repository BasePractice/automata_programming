cmake_minimum_required(VERSION 3.2.1)
project(09.FSM_SimInTech_Program)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)

set(HEADER_FILES delay_fsm.h)
set(SOURCE_FILES delay_fsm.c)
set(TESTS_SOURCE_FILES main_tests.cpp)

add_executable(${PROJECT_NAME}_Delay delay_main.c ${SOURCE_FILES})
add_executable(${PROJECT_NAME}_Tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME}_Tests tests_main)


