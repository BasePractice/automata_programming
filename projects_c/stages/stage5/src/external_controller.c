#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "emulator.h"

enum State
external_controller_read(const char *file_name, int *ok) {
    FILE *fd;
    enum State state = _Control;

    fd = fopen(file_name, "r");
    (*ok) = 0;
    if (fd != 0) {
        char buffer[20];
        size_t ret = fread(buffer, 1, sizeof(buffer), fd);
        buffer[ret] = 0;
        fclose(fd);
        if (!stricmp(buffer, "PowerOff")) {
            state = _PowerOff;
            (*ok) = 1;
        } else if (!stricmp(buffer, "PowerOn")) {
            state = _PowerOn;
            (*ok) = 1;
        }

        if ((*ok)) {
            remove(file_name);
        }
    }
    return state;
}

void
external_controller_write(const char *file_name, enum State state, int *ok) {
    (*ok) = 0;
    char *text_state = 0;
    switch (state) {
        case _PowerOff:
            text_state = "PowerOff";
            (*ok) = 1;
            break;
        case _PowerOn:
            text_state = "PowerOn";
            (*ok) = 1;
            break;
        default:
            text_state = 0;
            break;
    }

    if (text_state != 0) {
        FILE *fd = fopen(file_name, "w+");
        (*ok) = 0;
        if (fd != 0) {
            fwrite(text_state, 1, strlen(text_state), fd);
            fclose(fd);
            (*ok) = 1;
        }
    }
}