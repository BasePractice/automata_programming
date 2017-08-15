cmake_minimum_required(VERSION 3.4.1)
project(conditioner4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/.build")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
        @ONLY
)

set(HEADER_FILES src/conditioner_i.h src/emulator.h)
set(SOURCE_FILES src/conditioner.c src/memory_storage.c src/temperature_sensor.c)
set(MAIN_SOURCE_FILES src/main.c src/temperature_sensor.c)
set(TESTS_SOURCE_FILES tests/conditioner_i_tests.cpp tests/memory_storage_tests.cpp tests/temperature_sensor_tests.cpp)

add_executable(${CMAKE_PROJECT_NAME}4 ${SOURCE_FILES} ${MAIN_SOURCE_FILES} ${HEADER_FILES})
add_executable(${CMAKE_PROJECT_NAME}4_tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME}4_tests gtest gtest_main)
