# Лекция 10 — машина Тьюринга: интерпретатор ленты, программы, тесты.
project(10-turing-machine)

statecraft_library(turing_machine
        SOURCES turing_machine.c turing_machine.h mt_programs.c mt_programs.h)

# Пошаговый прогон и каталог машин для терминальной программы: своя лента и
# история, без учебных ограничений turing_machine.c (см. mt_session.h).
statecraft_library(mt_session
        SOURCES mt_session.c mt_session.h mt_catalog.c)

# Машина Поста (раздел лекции 10): интерпретатор с откатом и каталог программ;
# им пользуются и консольный инструмент, и терминальная программа.
statecraft_library(post_machine
        SOURCES post_machine.c post_machine.h pm_catalog.c)

statecraft_example(10-post-machine
        SOURCES post_main.c
        LIBS post_machine)

statecraft_example(${PROJECT_NAME}
        SOURCES main.c
        LIBS turing_machine)

statecraft_example(${PROJECT_NAME}-algorithm
        SOURCES change_sharp_algorithm.c)

statecraft_tests(${PROJECT_NAME}-tests
        SOURCES turing_machine_test.cpp mt_programs_test.cpp mt_session_test.cpp
                post_machine_test.cpp
        LIBS turing_machine mt_session post_machine)

if (STATECRAFT_BUILD_TESTS)
    # Пример лекции: «выполнив 61 команду; на ленте остаются две метки».
    add_test(NAME 10-post-machine-lecture
            COMMAND 10-post-machine --program 1)
    set_tests_properties(10-post-machine-lecture PROPERTIES
            PASS_REGULAR_EXPRESSION "на такте 61 \\(строка 9\\), меток 2")
endif ()

# Терминальная программа — необязательная цель, как «Жизнь» и умный муравей
# практик 11: общий терминальный слой tui_term заводится в
# practices/CMakeLists.txt только при STATECRAFT_BUILD_TUI на POSIX-системе.
if (TARGET tui_term)
    statecraft_example(${PROJECT_NAME}-tui
            SOURCES mt_tui.c
            LIBS mt_session tui_term)
    if (STATECRAFT_BUILD_TESTS)
        # Без терминала программа обязана сказать об этом и выйти.
        add_test(NAME ${PROJECT_NAME}-tui-no-tty
                COMMAND ${PROJECT_NAME}-tui)
        set_tests_properties(${PROJECT_NAME}-tui-no-tty PROPERTIES
                PASS_REGULAR_EXPRESSION "нужен терминал")
        # Снимок кадра терминала не требует: им проверяется отрисовка всех
        # машин каталога в окнах разного размера.
        add_test(NAME ${PROJECT_NAME}-tui-shots
                COMMAND ${PROJECT_NAME}-tui --check-shots)
    endif ()

    statecraft_example(10-post-machine-tui
            SOURCES pm_tui.c
            LIBS post_machine tui_term)
    if (STATECRAFT_BUILD_TESTS)
        add_test(NAME 10-post-machine-tui-no-tty
                COMMAND 10-post-machine-tui)
        set_tests_properties(10-post-machine-tui-no-tty PROPERTIES
                PASS_REGULAR_EXPRESSION "нужен терминал")
        add_test(NAME 10-post-machine-tui-shots
                COMMAND 10-post-machine-tui --check-shots)
    endif ()
endif ()
