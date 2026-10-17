# practices — примеры лекций и практические проекты курса.
#
# Подключается из корневого CMakeLists.txt. Автономная сборка тоже работает:
#   cmake -S practices -B build-practice && cmake --build build-practice
#
# Весь C-код собирается как ISO C90 без расширений и без предупреждений
# (см. cmake/StatecraftStrictC.cmake). Тесты — C++11 + Catch2.

cmake_minimum_required(VERSION 3.16)
project(statecraft_practice
        DESCRIPTION "Практикум курса АПСУ на C90"
        LANGUAGES C CXX)

# Автономный запуск: подтянуть общие модули и опции корня.
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
    option(STATECRAFT_BUILD_TESTS "Собирать тесты практикума (Catch2)" ON)
    option(STATECRAFT_STRICT "Предупреждения компилятора = ошибки" ON)
    include(StatecraftStrictC)
    if (STATECRAFT_BUILD_TESTS)
        enable_testing()
    endif ()
endif ()

add_definitions(-DEMPLOYEE="Student")

include_directories(common)

# --- общее представление автоматов -----------------------------------------
# Структуры НКА и ДКА плюс ввод-вывод: практики лекций 4 и 5 обмениваются
# автоматами через них, алгоритмы живут в самих практиках.
add_library(fsm STATIC common/fsm.c common/fsm.h)
target_include_directories(fsm PUBLIC common)
statecraft_strict_c(fsm)

# --- терминальный слой -----------------------------------------------------
# Терминальные программы практик 10 и 11 (машина Тьюринга, «Жизнь», умный
# муравей) — необязательные цели. Общий слой (common/tui_term.c) построен на
# termios и ioctl: это POSIX, а не ISO C90, и на Windows их нет. Заводится
# здесь, а не в практике, потому что подпроекты конфигурируются по порядку и
# практика 10 не увидела бы цель, объявленную в 11. Логика программ лежит в
# библиотеках практик и проверяется тестами при любом значении опции.
option(STATECRAFT_BUILD_TUI "Собирать терминальные программы практик 10 и 11 (нужен POSIX-терминал)" OFF)
if (STATECRAFT_BUILD_TUI)
    if (UNIX)
        add_library(tui_term STATIC common/tui_term.c common/tui_term.h)
        target_include_directories(tui_term PUBLIC common)
        statecraft_strict_c(tui_term)
    else ()
        message(WARNING "терминал POSIX недоступен, терминальные программы практик 10 и 11 пропущены")
    endif ()
endif ()

# --- общая часть тестов -----------------------------------------------------
if (STATECRAFT_BUILD_TESTS)
    add_library(tests_main STATIC common/tests_main.cpp common/catch2/catch.hpp)
    target_include_directories(tests_main PUBLIC common)
    statecraft_test_cxx(tests_main)
endif ()

# Пример или практический проект: statecraft_example(<цель> SOURCES ...)
function(statecraft_example target)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDES;LIBS;DEFINES" ${ARGN})
    add_executable(${target} ${ARG_SOURCES})
    if (ARG_INCLUDES)
        target_include_directories(${target} PRIVATE ${ARG_INCLUDES})
    endif ()
    if (ARG_LIBS)
        target_link_libraries(${target} PRIVATE ${ARG_LIBS})
    endif ()
    if (ARG_DEFINES)
        target_compile_definitions(${target} PRIVATE ${ARG_DEFINES})
    endif ()
    statecraft_strict_c(${target})
endfunction()

# Библиотека примера (когда её же используют тесты и main).
function(statecraft_library target)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDES;DEFINES" ${ARGN})
    add_library(${target} STATIC ${ARG_SOURCES})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${ARG_INCLUDES})
    if (ARG_DEFINES)
        target_compile_definitions(${target} PUBLIC ${ARG_DEFINES})
    endif ()
    statecraft_strict_c(${target})
endfunction()

# Тесты примера + регистрация в ctest.
function(statecraft_tests target)
    if (NOT STATECRAFT_BUILD_TESTS)
        return()
    endif ()
    cmake_parse_arguments(ARG "" "WORKDIR" "SOURCES;INCLUDES;LIBS" ${ARGN})
    add_executable(${target} ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE tests_main ${ARG_LIBS})
    if (ARG_INCLUDES)
        target_include_directories(${target} PRIVATE ${ARG_INCLUDES})
    endif ()
    statecraft_test_cxx(${target})
    add_test(NAME ${target} COMMAND ${target})
    if (ARG_WORKDIR)
        set_tests_properties(${target} PROPERTIES WORKING_DIRECTORY "${ARG_WORKDIR}")
    endif ()
endfunction()

# Каталоги названы по номеру лекции, к которой относится практика.
add_subdirectory(01-words)
add_subdirectory(02-fsm-delay)
add_subdirectory(03-synthesis)
add_subdirectory(03-control-program)
add_subdirectory(04-dfa-nfa)
add_subdirectory(05-kleene)
add_subdirectory(06-regular-expression)
add_subdirectory(06-lexical-analyze)
add_subdirectory(07-simple-program)
add_subdirectory(07-fsm-insert-element)
add_subdirectory(07-three-ways)
add_subdirectory(08-fsm-control-pneumo)
add_subdirectory(08-bulk-loading)
# У моделей Promela своей сборки нет: их проверяет внешний инструмент (SPIN),
# и подпроект добавляет только тесты, а без инструмента пропускается.
add_subdirectory(09-promela)
add_subdirectory(10-turing-machine)
add_subdirectory(11-cells)
add_subdirectory(11-ant-search)
add_subdirectory(12-takt)
add_subdirectory(20-welding-line)
add_subdirectory(21-additional)
