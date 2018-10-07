cmake_minimum_required(VERSION 3.2.1)
project(05.Lexical_Analyze)


include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)

set(HEADER_FILES mathematics_lang.h)
set(SOURCE_FILES mathematics_lang.c)
set(MAIN_SOURCE_FILES main.c)
set(TESTS_SOURCE_FILES mathematics_lang_tests.cpp)

add_executable(${PROJECT_NAME} ${MAIN_SOURCE_FILES})
add_executable(${PROJECT_NAME}_Tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME}_Tests tests_main)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests_data/__lexer_source.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

