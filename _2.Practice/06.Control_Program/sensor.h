#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/** Дискретные значения не нулевые */
enum Sensor {
    SENSOR_POWER_OFF, /* 1)  Тумблер вкл/выкл */
    SENSOR_POINT_PRESENT, /*2)  Деталь присутствует */

    SENSOR_D1, /* 3)  Начало изделия на конвейере*/
    SENSOR_D2, /* 4)  Окончание изделия на конвейфере */
    SENSOR_D3, /* 5)  Начало ширины изделия */
    SENSOR_D4, /* 6)  Окончание ширины изделия */
    SENSOR_D5, /* 7)  Высота изделия */

    SENSOR_M1, /* 8)  Концевой полодения слева */
    SENSOR_M2, /* 9)  Концевой положение справа */
    SENSOR_M3, /* 10) Концевой положение сверху */
    SENSOR_M4, /* 11) Концевой положения снизу */

    SENSOR_S1,  /* 12) Датчик положение шага сварки */



    LAST_SENSOR
};

struct SensorInterface {

    int (*get_value)(enum Sensor sensor);

    char *(*get_name)(enum Sensor sensor);

    void (*simulate_update)();
};

void emulator_sensor_init(struct SensorInterface *);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_SENSOR_H
