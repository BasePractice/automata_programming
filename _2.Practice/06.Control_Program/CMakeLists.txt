cmake_minimum_required(VERSION 3.2.1)
project(06.Control_Program)


option(ENABLE_NETWORK_EMULATE "Эмуляция показания датчиков по сети" OFF)
option(ENABLE_FILE_EMULATE "Эмуляция показания датчиков через файл" OFF)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(.)
include_directories(src)


set(HEADER_FILES sensor.h engine.h device.h properties.h)
set(SOURCE_FILES sensor.c engine.c device.c properties.c)
set(MAIN_SOURCE_FILES main.c)
#set(TESTS_SOURCE_FILES mathematics_lang_tests.cpp)

if (ENABLE_NETWORK_EMULATE)
    include_directories(../00.Base/network)

    add_definitions(-DNETWORK_EMULATE -DNETWOR_EMULATE_PORT="8097")
    set(EMULATE_SOURCES ../00.Base/network/eth_server.c)
    if (WIN32)
        set(EMULATE_LIBRARIES ws2_32)
    else ()
        set(EMULATE_LIBRARIES pthread)
    endif ()

    add_executable(${PROJECT_NAME}_emulate_network_cli emulate_network_cli.c)
    target_link_libraries(${PROJECT_NAME}_emulate_network_cli ${EMULATE_LIBRARIES})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/registers.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
elseif(ENABLE_FILE_EMULATE)
    add_definitions(-DFILE_EMULATE)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests/registers.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()

add_executable(${PROJECT_NAME} ${MAIN_SOURCE_FILES} ${SOURCE_FILES} ${EMULATE_SOURCES})
#add_executable(${PROJECT_NAME}_Tests ${TESTS_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(${PROJECT_NAME} ${EMULATE_LIBRARIES})


