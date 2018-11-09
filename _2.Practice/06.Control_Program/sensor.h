#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Дискретные значения не нулевые */
enum Sensor {
    SENSOR_POWER_OFF, /* Тумблер вкл/выкл */

    SENSOR_D1, /* Начало изделия на конвейере*/
    SENSOR_D2, /* Окончание изделия на конвейфере */
    SENSOR_D3, /* Начало ширины изделия */
    SENSOR_D4, /* Окончание ширины изделия */
    SENSOR_D5, /* Высота изделия */

    SENSOR_M1, /* Концевой полодения слева */
    SENSOR_M2, /* Концевой положение справа */
    SENSOR_M3, /* Концевой положение сверху */
    SENSOR_M4, /* Концевой положения снизу */

    SENSOR_S1  /* Датчик положение шага сварки */
};

struct SensorInterface {

    int (*get_value)(enum Sensor sensor);

    char *(*get_name)(enum Sensor sensor);
};

void emulator_sensor_init(struct SensorInterface *);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H
