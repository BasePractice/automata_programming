#include "properties.h"

static int get_integer(enum Property p) {
    switch (p) {
        case VerticalStepsCenter:
            return 10;
        case VerticalStepsPoint1:
            return 3;
        case VerticalStepsPoint2:
            return -3;
        case VerticalStepsPoint3:
            return -2;
    }
    return 0;
}

void
emulator_properties_init(struct PropertyInterface *pi) {
    pi->get_integer = get_integer;
}