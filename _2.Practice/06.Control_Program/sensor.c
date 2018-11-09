#include <stdio.h>
#include "sensor.h"

static int get_value(enum Sensor sensor) {
    return 1;
}

static char *get_name(enum Sensor sensor) {
    static char buf[20];
    switch (sensor) {
        case SENSOR_D1:
        case SENSOR_D2:
        case SENSOR_D3:
        case SENSOR_D4:
        case SENSOR_D5:
        case SENSOR_M1:
        case SENSOR_M2:
        case SENSOR_M3:
        case SENSOR_M4:
        case SENSOR_S1:
        case SENSOR_POWER_OFF:
            sprintf(buf, "Sensor %02d", sensor);
            return buf;
    }
}

void
emulator_sensor_init(struct SensorInterface *si) {
    si->get_name = get_name;
    si->get_value = get_value;
}
