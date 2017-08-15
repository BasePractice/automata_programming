#ifndef _CONDITIONER_I_H_
#define _CONDITIONER_I_H_

#if defined(_MSC_VER)
#include <windows.h>
#define x_sleep(t)   Sleep(t)
#else
#include <unistd.h>
#define x_sleep(t)   usleep((t) * 1000)
#endif

/** Параметры кондиционера */
enum Property {
    Temperature,
    WaitingCooling,
    WaitingHeating,
    WaitingIdle,
    _PropertyCount
};

/** Режим работы кондиционера */
enum EngineMode {
    /** Охлаждение */
    Cooling,
    /** Нагрев */
    Heating,
    /** Ожидание */
    Waiting
};

struct BaseInter {
    int (*errors)();
};

/** Интерфейс хранилища */
struct StorageInter {
    struct BaseInter base;
    /** Загрузка параметров их хранилища */
    int (*property_load)();
    /** Сохранение параметров */
    int (*property_store)();
    int  (*property_get_int)(enum Property key);
    /** Установка параметра по его наименованию */
    void (*property_set_int)(enum Property key, int value);
};

/** Основной интерфейс кондициронер */
struct ConditionerInter {
    struct BaseInter base;
    /** Получение температуры помещения */
    int (*get_current_temperature)();
    /** Выставление работы с установкой режима работы и времени работы */
    int (*process_engine)(enum EngineMode mode, int timeout);
    /** Добавить проверку наличия ошибок */

    int (*control)(int state);
};

#endif
