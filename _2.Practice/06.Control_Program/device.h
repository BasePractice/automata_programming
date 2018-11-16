#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H

#if defined(__cplusplus)
extern "C" {
#endif

enum Device {
    DEVICE_POWER_OFF,
    DEVICE_CATCH_LINE, //Захват конвейера
    DEVICE_FREE_LINE, //Захват конвейера
    DEVICE_POINT_POSITION,

    DEVICE_WELDING_UP, DEVICE_WELDING_DOWN,
    DEVICE_WELDING_LEFT, DEVICE_WELDING_RIGHT,
    DEVICE_WELDING_NOP, DEVICE_WELDING /* Сварка  */,

    DEVICE_DROP_OBJECT,

    MT1, /* Включение ШД для конвейера  */

    MT2, /* Шаг налево  */
    MT3, /* Шаг направо  */
    MT4, /* Шаг вверх  */
    MT5, /* Шаг вниз  */
};

struct DeviceInterface {
    void (*do_step)(enum Device device);
    void (*print)(char *text);
};

void emulator_device_init(struct DeviceInterface *di);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H
