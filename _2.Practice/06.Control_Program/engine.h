#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H

#include <properties.h>
#include <sensor.h>
#include <device.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum EngineState {
    ENGINE_OFF, ENGINE_ON
};

void engine_execute(
        struct PropertyInterface *pi,
        struct SensorInterface *si,
        struct DeviceInterface *di);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_ENGINE_H
