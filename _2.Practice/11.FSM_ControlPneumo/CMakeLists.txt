cmake_minimum_required(VERSION 3.2.1)
project(11.FSM_ControlPneumo)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)

add_definitions(-DPNEUMO_DEBUG -DPNEUMO_SIMULATE)

add_library(${PROJECT_NAME}_Library STATIC pneumo_ctrl.c pneumo_ctrl.h)
add_executable(${PROJECT_NAME}_Main main.c)
target_link_libraries(${PROJECT_NAME}_Main ${PROJECT_NAME}_Library)