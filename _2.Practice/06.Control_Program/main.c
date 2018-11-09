#include <stdlib.h>
#include <stdio.h>
#include "properties.h"
#include "engine.h"
#include "device.h"
#include "sensor.h"


int main(int argc, char **argv) {
    struct SensorInterface si;
    struct DeviceInterface di;
    struct PropertyInterface pi;

    emulator_properties_init(&pi);
    emulator_sensor_init(&si);
    emulator_device_init(&di);

    engine_execute(&pi, &si, &di);
    return EXIT_SUCCESS;
}

