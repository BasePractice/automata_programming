cmake_minimum_required(VERSION 3.4.1)
project(conditioner)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.build")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CONDITIONER_PRODUCT_NUMBER  0)
set(CONDITIONER_PRODUCT_VERSION 0)
set(CONDITIONER_BUILD_NUMBER    1)

include_directories(3rdparty/gtest/googletest/include)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

if (MSVC)
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} /SUBSYSTEM:CONSOLE /IGNORE:4099")
    if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(CMAKE_CFLAGS "${CMAKE_CFLAGS} /MDd /bigobj")
        add_compile_options(/MDd /bigobj)
    else ()
        add_definitions(/MT)
        add_compile_options(/bigobj)
    endif ()
    add_definitions(/wd4267)
    add_definitions(/wd4250)
    add_definitions(/wd4996)
elseif (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pedantic -Wp,-w -Wa,-mbig-obj")
    set(CMAKE_CFLAGS "${CMAKE_CFLAGS} -pedantic -Wp,-w -Wa,-mbig-obj")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pedantic -Wp,-w")
    set(CMAKE_CFLAGS "${CMAKE_CFLAGS} -pedantic -Wp,-w")
endif ()

add_subdirectory(3rdparty/gtest/googletest)
add_definitions(
        -DTESTS_DATA_DIRECTORY="tests_data/"
)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.h
        @ONLY
)

set(SOURCE_FILES src/conditioner.c src/memory_storage.c)
set(MAIN_SOURCE_FILES src/main.c)
set(TESTS_SOURCE_FILES tests/conditioner_i_tests.cpp tests/memory_storage_tests.cpp)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES} ${MAIN_SOURCE_FILES} )
add_executable(${CMAKE_PROJECT_NAME}_tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES})
target_link_libraries(${CMAKE_PROJECT_NAME}_tests gtest gtest_main)
file(COPY
        ${CMAKE_SOURCE_DIR}/tests_data/conditioner_001.blob
        DESTINATION
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests_data)
file(COPY
        ${CMAKE_SOURCE_DIR}/tests_data/conditioner_main.blob
        DESTINATION
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests_data)
