#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_PROPERTIES_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_PROPERTIES_H

#if defined(__cplusplus)
extern "C" {
#endif

enum Property {
    VerticalStepsCenter, /* Количество шагов сварки от левой крайней точки до правой крайней точки */

    VerticalStepsPoint1,
    VerticalStepsPoint2,
    VerticalStepsPoint3,
};

struct PropertyInterface {
    int (*get_integer)(enum Property p);
};

void emulator_properties_init(struct PropertyInterface *pi);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_PROPERTIES_H
