#include <stdbool.h>
#include <stdio.h>
#include "engine.h"

void engine_execute(
        struct PropertyInterface *pi,
        struct SensorInterface *si,
        struct DeviceInterface *di) {
    enum EngineState state = ENGINE_ON;
    bool running = true;

    while (running) {
        switch (state) {
            case ENGINE_OFF:
                (*di->do_step)(DEVICE_POWER_OFF);
                running = false;
                fprintf(stdout, "Engine power off\n");
                break;
            case ENGINE_ON: {
                fprintf(stdout, "Engine start\n");
                break;
            }
        }

        /* Получаем данные о том выключено устройтсво или нет */
        if ((*si->get_value)(SENSOR_POWER_OFF)) {
            state = ENGINE_OFF;
        }
    }
}

