# Лекция 12 — язык Takt: модели, порождение C, драйвер на C90.
#
# Инструменты Takt (`taktc` и `takt-sim`) внешние по отношению к курсу. Каталог
# с ними задаётся переменной STATECRAFT_TAKT_DIR — как переменной CMake, так и
# переменной окружения; по умолчанию это ~/.local/bin. Если там нужных
# бинарников нет, они ищутся в PATH, а если не найдены и там — подпроект
# пропускается с сообщением: остальная сборка курса не должна зависеть от
# наличия чужого репозитория.
#
#   export STATECRAFT_TAKT_DIR=~/github/BuT/target/release
#   cmake -S . -B build -DSTATECRAFT_TAKT_DIR=/путь/к/каталогу
project(12-takt)

# --- где искать инструменты -------------------------------------------------
if (NOT STATECRAFT_TAKT_DIR AND DEFINED ENV{STATECRAFT_TAKT_DIR})
    set(STATECRAFT_TAKT_DIR "$ENV{STATECRAFT_TAKT_DIR}")
endif ()
# Прежнее имя переменной: оставлено, чтобы не ломать уже настроенные сборки.
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

set(TAKT_MODELS traffic watchdog button_led pump cooling_bad cooling_good recovery)

find_program(TAKTC_BIN NAMES taktc
        HINTS "${STATECRAFT_TAKT_DIR}"
        DOC "Компилятор языка Takt")
find_program(TAKT_SIM_BIN NAMES takt-sim
        HINTS "${STATECRAFT_TAKT_DIR}"
        DOC "Симулятор языка Takt")

# Инструменты нужны оба и одной версии: код порождает `taktc`, а сценарии
# прогоняет `takt-sim`, и расхождение версий даёт ошибки, которые выглядят как
# ошибки моделей. Поэтому половинчатая установка — повод сказать об этом вслух.
if (TAKTC_BIN AND NOT TAKT_SIM_BIN)
    message(WARNING "statecraft: taktc найден (${TAKTC_BIN}), а takt-sim — нет; "
            "сценарии симулятора прогоняться не будут. "
            "Ожидаемый каталог: ${STATECRAFT_TAKT_DIR}")
elseif (TAKT_SIM_BIN AND NOT TAKTC_BIN)
    message(WARNING "statecraft: takt-sim найден (${TAKT_SIM_BIN}), а taktc — нет; "
            "примеры Takt собираться не будут. "
            "Ожидаемый каталог: ${STATECRAFT_TAKT_DIR}")
endif ()

if (NOT TAKTC_BIN)
    message(STATUS "statecraft: taktc не найден в ${STATECRAFT_TAKT_DIR} и в PATH — "
            "примеры Takt пропущены "
            "(укажите -DSTATECRAFT_TAKT_DIR=<каталог с taktc и takt-sim>)")
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
    return()
endif ()

message(STATUS "statecraft: taktc — ${TAKTC_BIN}")
if (TAKT_SIM_BIN)
    message(STATUS "statecraft: takt-sim — ${TAKT_SIM_BIN}")
endif ()

set(TAKT_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${TAKT_GEN_DIR}")

# Проверка синтаксиса и семантики всех моделей: цель ошибается, если хоть одна
# модель перестала компилироваться. Заодно порождается C для каждой из них.
set(TAKT_GENERATED "")
foreach (model IN LISTS TAKT_MODELS)
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/model/${model}.takt")
    set(out_c "${TAKT_GEN_DIR}/${model}.c")
    set(out_h "${TAKT_GEN_DIR}/${model}.h")
    add_custom_command(
            OUTPUT "${out_c}" "${out_h}"
            COMMAND "${TAKTC_BIN}" compile --target c "${src}" -o "${TAKT_GEN_DIR}/"
            DEPENDS "${src}"
            COMMENT "taktc: ${model}.takt → C"
            VERBATIM)
    list(APPEND TAKT_GENERATED "${out_c}" "${out_h}")
endforeach ()

add_custom_target(${PROJECT_NAME}-models DEPENDS ${TAKT_GENERATED})

# Порождённый код — C99 (uint8_t, bool, комментарии //), поэтому общий режим
# C90 к нему неприменим: это выход чужого компилятора, а не учебный код.
add_library(takt_generated STATIC
        "${TAKT_GEN_DIR}/watchdog.c"
        "${TAKT_GEN_DIR}/traffic.c")
target_include_directories(takt_generated PUBLIC "${TAKT_GEN_DIR}")
statecraft_generated_c99(takt_generated)

# Драйвер включает порождённый заголовок, а значит тоже собирается как C99:
# C90 не знает ни bool, ни uint8_t. Это единственная цель курса вне общего
# режима, и причина у неё внешняя — интерфейс чужого генератора.
add_executable(${PROJECT_NAME}-watchdog main.c)
target_link_libraries(${PROJECT_NAME}-watchdog PRIVATE takt_generated)
statecraft_generated_c99(${PROJECT_NAME}-watchdog)

if (STATECRAFT_BUILD_TESTS)
    add_test(NAME ${PROJECT_NAME}-watchdog COMMAND ${PROJECT_NAME}-watchdog)
endif ()

# Оформление моделей задаёт сам язык, и канон исполним командой: `taktc fmt`.
# Тест проверяет, не разошлись ли модели с ним, — иначе стиль расползается
# незаметно, как это уже было с версией компилятора (см. CLAUDE.md, ТД-3).
if (STATECRAFT_BUILD_TESTS)
    add_test(NAME ${PROJECT_NAME}-format
            COMMAND "${TAKTC_BIN}" fmt --check "${CMAKE_CURRENT_SOURCE_DIR}/model")
endif ()

# Сценарий симулятора — тест самой модели, а не порождённого кода: он подаёт
# входы по тактам и проверяет состояние после каждого (`guard`).
if (TAKT_SIM_BIN AND STATECRAFT_BUILD_TESTS)
    add_test(NAME ${PROJECT_NAME}-scenario
            COMMAND "${TAKT_SIM_BIN}"
            "${CMAKE_CURRENT_SOURCE_DIR}/model/watchdog.takt"
            -s "${CMAKE_CURRENT_SOURCE_DIR}/scenario/watchdog.json")
endif ()
