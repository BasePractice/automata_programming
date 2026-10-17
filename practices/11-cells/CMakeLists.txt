# Лекция 11 — клеточные автоматы: правила Вольфрама, «Жизнь», муравей Лэнгтона.
project(11-cells)

statecraft_library(cells
        SOURCES cells.c cells.h
                ant_trail.c ant_trail.h
                render.c render.h
                life_session.c life_session.h)

# Векторный вывод (render.c) считает координаты через cos, sin и sqrt. На
# macOS они лежат в libc, а на Linux — в отдельной libm, и без явной
# линковки сборка падает на этапе компоновки. Нашлось в CI: на машине
# разработчика этого не видно.
find_library(STATECRAFT_M_LIBRARY m)
if (STATECRAFT_M_LIBRARY)
    target_link_libraries(cells PUBLIC "${STATECRAFT_M_LIBRARY}")
endif ()

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS cells)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES cells_tests.cpp life_session_tests.cpp
        LIBS cells)

# Терминальная «Жизнь» (life_tui.c) — необязательная цель. Режим терминала
# задаётся через termios и ioctl: это POSIX, а не ISO C90, и на Windows их
# нет. Логика программы (поле, история, каталог объектов) лежит в библиотеке
# cells и проверяется тестами при любом значении опции.
option(STATECRAFT_BUILD_TUI "Собирать терминальную игру «Жизнь» (нужен POSIX-терминал)" OFF)
if (STATECRAFT_BUILD_TUI)
    if (UNIX)
        statecraft_example(${PROJECT_NAME}-life-tui
                SOURCES life_tui.c
                LIBS cells)
        # Без терминала программа обязана сказать об этом и выйти, а не
        # испортить вывод управляющими последовательностями: ctest как раз
        # запускает её без терминала.
        if (STATECRAFT_BUILD_TESTS)
            add_test(NAME ${PROJECT_NAME}-life-tui-no-tty
                    COMMAND ${PROJECT_NAME}-life-tui)
            set_tests_properties(${PROJECT_NAME}-life-tui-no-tty PROPERTIES
                    PASS_REGULAR_EXPRESSION "нужен терминал")
        endif ()
    else ()
        message(WARNING "11-cells-life-tui: терминал POSIX недоступен, цель пропущена")
    endif ()
endif ()
