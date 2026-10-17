# Лекция 11 — поиск автомата для задачи об умном муравье.
#
# Тропа, прогон и готовые стратегии берутся из практики 11-cells (библиотека
# cells): карта тропы обязана быть одна на весь курс, иначе числа тактов в
# лекции и в тестах разойдутся.
project(11-ant-search)

statecraft_library(ant_search
        SOURCES ant_search.c ant_search.h
                ant_ep93.c ant_ep93.h
                ant_random.c ant_random.h
                ant_session.c ant_session.h)
target_link_libraries(ant_search PUBLIC cells)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS ant_search)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES ant_search_tests.cpp ant_session_tests.cpp
        LIBS ant_search)

# Терминальный умный муравей — необязательная цель, как и «Жизнь» в
# 11-cells: общий терминальный слой tui_term заводится там же и только при
# STATECRAFT_BUILD_TUI на POSIX-системе.
if (TARGET tui_term)
    statecraft_example(${PROJECT_NAME}-tui
            SOURCES ant_tui.c
            LIBS ant_search tui_term)
    if (STATECRAFT_BUILD_TESTS)
        add_test(NAME ${PROJECT_NAME}-tui-no-tty
                COMMAND ${PROJECT_NAME}-tui)
        set_tests_properties(${PROJECT_NAME}-tui-no-tty PROPERTIES
                PASS_REGULAR_EXPRESSION "нужен терминал")
    endif ()
endif ()
