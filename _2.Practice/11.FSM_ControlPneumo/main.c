#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "pneumo_ctrl.h"

int main(int argc, char **argv) {
    FILE *fd;
    if (argc < 2) {
        fprintf(stderr, "Не передан файл симуляции\n");
        return EXIT_FAILURE;
    }
    fd = fopen(argv[1], "rt");
    if (0 != fd) {
        struct PneumoEngine engine = {};
        bool running = true;

        pneumo_engine_init(&engine);
        while (running) {
            int eq_output[2];
            if (feof(fd)) {
                fprintf(stdout, "Завершение файла симуляции\n");
                running = false;
                continue;
            }
            fscanf(fd, "%d %d %d %d %d %d",
                   (int *) &engine.cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP],
                   (int *) &engine.cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN],
                   (int *) &engine.cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_UP],
                   (int *) &engine.cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN],
                   (int *) &eq_output[PNEUMO_CYLINDER_Y1],
                   (int *) &eq_output[PNEUMO_CYLINDER_Y2]
            );
            running = pneumo_engine_tick(&engine);
            if (!running) {
                fprintf(stderr, "Остановка из-за критической ошибки автомата\n");
            }
        }
        pneumo_engine_destroy(&engine);
    }
    if (0 != fd) {
        fclose(fd);
    }
    return EXIT_SUCCESS;
}
