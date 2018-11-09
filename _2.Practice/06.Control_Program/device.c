#include "device.h"

static void do_step(enum Device device) {

}

void
emulator_device_init(struct DeviceInterface *di) {
    di->do_step = do_step;
}

