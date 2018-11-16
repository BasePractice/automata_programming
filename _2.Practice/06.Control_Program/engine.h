#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H

#include <properties.h>
#include <sensor.h>
#include <device.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum EngineState {
    ENGINE_OFF, ENGINE_ON,
    ENGINE_ERROR,

    ENGINE_WAIT_OBJECT,
    ENGINE_BEGIN_WORK, //Ожидаем изделие, пока оно не будет спозиционировано, далее блокируем ленту
    ENGINE_POSITION, //Позиционирование сварочного аппарата к центру изделия
    ENGINE_FIND_NEXT_POINT, //Поиск следующей точки
    ENGINE_FIND_PROCESS, //Процесс поиска точки
    ENGINE_POINT_NOT_FOUND, //Точка не найдена, удаляем изделия с конвеера
    ENGINE_CONCRETE_NEXT_POINT_POSITION, //Позиционирование сварки над точкой
    ENGINE_WELDING_DOWN,
    ENGINE_WELDING_UP,
    ENGINE_WELDING, //Процесс сварки
    ENGINE_START_POSITION, //Начальное положение сварки
    ENGINE_FREE, //Освобождение конвеера
    ENGINE_WAIT_FREE_OBJECT, //Ожидание пока изделие не удет с конвейера

    ENGINE_MOVE_WELDING_DEVICE //Состояние перемещения сварки
};

void engine_execute(
        struct PropertyInterface *pi,
        struct SensorInterface *si,
        struct DeviceInterface *di);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H
