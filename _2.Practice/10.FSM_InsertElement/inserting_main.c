#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inserting_fsm.h"

int main(int argc, char **argv) {
    struct InsertingEngine engine = {INSERTING_A};
    size_t sequence_it;
    bool running = true;

    if (argc < 3) {
        fprintf(stderr, "Передайте в качестве аргументов две последовательности символов\n");
        return EXIT_FAILURE;
    }
    fprintf(stdout, "1: %s\n", argv[1]);
    fprintf(stdout, "2: %s\n", argv[2]);
    inserting_init(&engine, argv[1], strlen(argv[1]), argv[2], strlen(argv[2]));
    for (sequence_it = 0; running; ++sequence_it) {
        enum InsertingEvent event = inserting_engine(&engine);
        switch (event) {
            case INSERTING_ERROR_END:
                fprintf(stderr, "Ошибка проверки. \n");
                running = false;
                break;
            case INSERTING_OK_END:
                fprintf(stdout, "Проверка завершилась. \n");
                running = false;
                break;
            case INSERTING_DETECT:
                fprintf(stdout, "Обнаружена вставка на участке %llu до %llu. \n", engine.c_1, engine.c_2);
                break;
            case INSERTING_DETECT_END:
                fprintf(stdout, "Обнаружена вставка на участке %llu до %llu. \n", engine.c_1, engine.c_2);
                running = false;
                break;
            case INSERTING_NEXT:
            default:
                break;
        }
    }
    fprintf(stdout, "\n");
    return EXIT_SUCCESS;
}
