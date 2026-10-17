# cmake/StatecraftTakt.cmake
#
# Проверка того, что найденный компилятор Takt порождает C в той форме, на
# которую написаны драйверы курса (practices/12-takt/main.c,
# practices/21-additional/loader/loader_runner.c).
#
#   statecraft_takt_usable(<taktc> <результат>)
#
# Зачем. Takt развивается отдельно от курса, и форма порождённого C меняется.
# С доработки 0195 перечислители портов разведены с перечислителями
# состояний сегментом PORT_: <МОДЕЛЬ>_PORT_<ПОРТ>. Компилятор старше неё
# порождает <МОДЕЛЬ>_<МОДЕЛЬ>_<ПОРТ>, и сборка драйвера падает ошибкой
# компилятора Си про необъявленный идентификатор — то есть выглядит как
# дефект курса, а не как устаревший инструмент. Проверка превращает это в
# предупреждение с указанием, что обновить, а Takt-практики пропускаются,
# как будто taktc не найден вовсе.
#
# Проба — компиляция модели сторожевого таймера из практики 12 во временный
# каталог и поиск сегмента PORT_ в заголовке. Модель берётся настоящая:
# если она перестанет компилироваться, это тоже повод не собирать практики.

set(STATECRAFT_TAKT_PROBE_MODEL
    "${CMAKE_CURRENT_LIST_DIR}/../practices/12-takt/model/watchdog.takt")

function(statecraft_takt_usable taktc result)
    set(probe_dir "${CMAKE_BINARY_DIR}/takt-probe")
    file(REMOVE_RECURSE "${probe_dir}")
    file(MAKE_DIRECTORY "${probe_dir}")
    execute_process(
        COMMAND "${taktc}" compile --target c "${STATECRAFT_TAKT_PROBE_MODEL}" -o "${probe_dir}/"
        RESULT_VARIABLE code
        OUTPUT_QUIET ERROR_VARIABLE errors)
    if (NOT code EQUAL 0 OR NOT EXISTS "${probe_dir}/watchdog.h")
        message(WARNING "statecraft: ${taktc} не компилирует модель "
                "practices/12-takt/model/watchdog.takt — примеры Takt пропущены.\n${errors}")
        set(${result} FALSE PARENT_SCOPE)
        return()
    endif ()
    file(STRINGS "${probe_dir}/watchdog.h" port_names REGEX "WATCHDOG_PORT_KICK")
    if (NOT port_names)
        message(WARNING "statecraft: ${taktc} устарел — порты в порождённом C названы "
                "без сегмента PORT_ (Takt старше доработки 0195), а драйверы курса "
                "написаны под <МОДЕЛЬ>_PORT_<ПОРТ>. Примеры Takt пропущены. "
                "Обновите taktc и takt-sim из ветки v2 репозитория "
                "https://github.com/Pastor/Takt и укажите каталог с ними: "
                "-DSTATECRAFT_TAKT_DIR=<каталог> (по умолчанию ~/.local/bin).")
        set(${result} FALSE PARENT_SCOPE)
        return()
    endif ()
    set(${result} TRUE PARENT_SCOPE)
endfunction()
