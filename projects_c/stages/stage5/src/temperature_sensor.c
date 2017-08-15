#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "emulator.h"

int
temperature_sensor_read(const char *file_name, int *ok) {
    FILE *fd;
    int value = 0;

    fd = fopen(file_name, "r");
    (*ok) = 1;
    if (fd != 0) {
        char buffer[20];
        size_t ret = fread(buffer, 1, sizeof(buffer), fd);
        buffer[ret] = 0;
        value = strtol(buffer, 0, 10);
        fclose(fd);
    } else {
        (*ok) = 0;
    }
    return value;
}

int
temperature_sensor_write(const char *file_name, int value, int *ok) {
    FILE *fd;
    int ret = 0;

    fd = fopen(file_name, "w+");
    (*ok) = 1;
    if (fd != 0) {
        char buffer[20];
        sprintf(buffer, "%d", value);
        fwrite(buffer, 1, strlen(buffer), fd);
        fclose(fd);
        ret = 1;
    } else {
        (*ok) = 0;
    }
    return ret;
}