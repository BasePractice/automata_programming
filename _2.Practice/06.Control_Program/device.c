#include <stdio.h>
#include "device.h"

static void do_step(enum Device device) {

}

static void print(char *text) {
    fprintf(stdout, "%s\n", text);
}

void
emulator_device_init(struct DeviceInterface *di) {
    di->do_step = do_step;
    di->print = print;
}

