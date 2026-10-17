# Транспортный погрузчик цеха: система управления на Takt, планировщик
# маршрута, конфигурация и модель установки на ISO C90.
#
# Каталог с `taktc` и `takt-sim` задаётся переменной STATECRAFT_TAKT_DIR — так
# же, как в practices/12-takt; по умолчанию это ~/.local/bin, затем PATH. Без
# компилятора Takt собирается всё, кроме самой системы управления.

statecraft_library(loader
        SOURCES factory_map.c factory_map.h
                route.c route.h
                loader_plant.c loader_plant.h
                scenario.c scenario.h)

statecraft_example(21-additional
        SOURCES main.c
        LIBS loader)

statecraft_tests(21-additional-tests
        SOURCES loader_tests.cpp
        LIBS loader
        WORKDIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Конфигурации нужны рядом с исполняемым файлом: инструмент читает их по
# относительному имени.
foreach (config factory.json factory-grid.json)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/${config}"
            "${CMAKE_CURRENT_BINARY_DIR}/${config}" COPYONLY)
endforeach ()

file(GLOB LOADER_MISSIONS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mission/*.json")
foreach (item IN LISTS LOADER_MISSIONS)
    get_filename_component(item_name "${item}" NAME)
    configure_file("${item}" "${CMAKE_CURRENT_BINARY_DIR}/mission/${item_name}" COPYONLY)
endforeach ()

# --- система управления на Takt ---------------------------------------------
if (NOT STATECRAFT_TAKT_DIR AND DEFINED ENV{STATECRAFT_TAKT_DIR})
    set(STATECRAFT_TAKT_DIR "$ENV{STATECRAFT_TAKT_DIR}")
endif ()
if (NOT STATECRAFT_TAKT_DIR AND STATECRAFT_TAKTC)
    set(STATECRAFT_TAKT_DIR "${STATECRAFT_TAKTC}")
endif ()
if (NOT STATECRAFT_TAKT_DIR AND DEFINED ENV{STATECRAFT_TAKTC})
    set(STATECRAFT_TAKT_DIR "$ENV{STATECRAFT_TAKTC}")
endif ()
if (NOT STATECRAFT_TAKT_DIR)
    if (DEFINED ENV{HOME})
        set(STATECRAFT_TAKT_DIR "$ENV{HOME}/.local/bin")
    elseif (DEFINED ENV{USERPROFILE})
        set(STATECRAFT_TAKT_DIR "$ENV{USERPROFILE}/.local/bin")
    endif ()
endif ()

find_program(TAKTC_BIN NAMES taktc HINTS "${STATECRAFT_TAKT_DIR}"
        DOC "Компилятор языка Takt")
find_program(TAKT_SIM_BIN NAMES takt-sim HINTS "${STATECRAFT_TAKT_DIR}"
        DOC "Симулятор языка Takt")

if (NOT TAKTC_BIN)
    message(STATUS "statecraft: taktc не найден — погрузчик собирается без "
            "системы управления (планировщик и модель цеха доступны)")
    return()
endif ()

include(StatecraftTakt)
statecraft_takt_usable("${TAKTC_BIN}" TAKTC_USABLE)
if (NOT TAKTC_USABLE)
    # Путь забывается: иначе обновлённый taktc (например, в ~/.local/bin) не
    # нашёлся бы при следующей конфигурации — find_program не ищет заново то,
    # что уже лежит в кэше.
    unset(TAKTC_BIN CACHE)
    unset(TAKT_SIM_BIN CACHE)
    message(STATUS "statecraft: погрузчик собирается без системы управления "
            "(планировщик и модель цеха доступны)")
    return()
endif ()

set(LOADER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${LOADER_GEN_DIR}")

add_custom_command(
        OUTPUT "${LOADER_GEN_DIR}/loader.c" "${LOADER_GEN_DIR}/loader.h"
        COMMAND "${TAKTC_BIN}" compile --target c
                "${CMAKE_CURRENT_SOURCE_DIR}/model/loader.takt" -o "${LOADER_GEN_DIR}/"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/model/loader.takt"
        COMMENT "taktc: loader.takt → C"
        VERBATIM)

# Порождённый код и всё, что включает его заголовок, — C99 (uint8_t, bool,
# комментарии //). Общий режим C90 к ним неприменим: это выход чужого
# компилятора, а не учебный код.
add_library(loader_runtime STATIC
        "${LOADER_GEN_DIR}/loader.c"
        "${LOADER_GEN_DIR}/loader.h"
        loader_runner.c loader_runner.h)
target_include_directories(loader_runtime PUBLIC "${LOADER_GEN_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(loader_runtime PUBLIC loader)
statecraft_generated_c99(loader_runtime)

add_executable(21-additional-run loader_main.c)
target_link_libraries(21-additional-run PRIVATE loader_runtime)
statecraft_generated_c99(21-additional-run)

if (STATECRAFT_BUILD_TESTS)
    add_test(NAME 21-additional-run
            COMMAND 21-additional-run --quiet
            --file "${CMAKE_CURRENT_SOURCE_DIR}/factory.json")
endif ()

# Оформление модели задаёт язык (`taktc fmt`), и расхождение с каноном ловится
# тестом, а не обзором. То же самое сделано в practices/12-takt.
if (STATECRAFT_BUILD_TESTS)
    add_test(NAME 21-additional-format
            COMMAND "${TAKTC_BIN}" fmt --check "${CMAKE_CURRENT_SOURCE_DIR}/model")
endif ()

# Сценарий симулятора проверяет саму модель — до того, как порождён код.
if (TAKT_SIM_BIN AND STATECRAFT_BUILD_TESTS
        AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scenario/loader.json")
    add_test(NAME 21-additional-scenario
            COMMAND "${TAKT_SIM_BIN}"
            "${CMAKE_CURRENT_SOURCE_DIR}/model/loader.takt"
            -s "${CMAKE_CURRENT_SOURCE_DIR}/scenario/loader.json")
endif ()
