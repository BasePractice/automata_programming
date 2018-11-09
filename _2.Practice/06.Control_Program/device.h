#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H

#if defined(__cplusplus)
extern "C" {
#endif

enum Device {
    DEVICE_POWER_OFF,

    MT1, /* Включение ШД для конвейера  */

    MT2, /* Шаг налево  */
    MT3, /* Шаг направо  */
    MT4, /* Шаг вверх  */
    MT5, /* Шаг вниз  */

    C1, /* Сварка  */
};

struct DeviceInterface {
    void (*do_step)(enum Device device);
};

void emulator_device_init(struct DeviceInterface *di);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_DEVICE_H
