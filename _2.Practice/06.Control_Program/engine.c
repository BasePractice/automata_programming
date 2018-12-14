#include <stdbool.h>
#include <stdio.h>
#include "engine.h"

void engine_execute(
        struct PropertyInterface *pi,
        struct SensorInterface *si,
        struct DeviceInterface *di) {
    enum EngineState state = ENGINE_ON;
    enum EngineState next_move_state = ENGINE_OFF;
    enum EngineState next_position_state = ENGINE_OFF;
    enum Device welding_step_direct = DEVICE_WELDING_NOP;
    bool running = true;
    int center_step_count = 0;
    int step_it = 0;
    int point_it = -1; //Номер текущей точки

    bool is_up = false;

    bool sensor_d1 = false;
    bool sensor_d2 = false;
    bool sensor_d3 = false;
    bool sensor_d4 = false;
    bool sensor_d5 = false;

    bool sensor_m1 = false;
    bool sensor_m2 = false;
    bool sensor_m3 = false;
    bool sensor_m4 = false;

    while (running) {
        switch (state) {
            case ENGINE_OFF:
                (*di->print)("Engine power off");
                (*di->do_step)(DEVICE_POWER_OFF);
                running = false;
                break;
            case ENGINE_ON: {
                (*di->print)("Engine start");
                center_step_count = (*pi->get_integer)(VerticalStepsCenter);

                next_position_state = ENGINE_WAIT_OBJECT;
                state = ENGINE_START_POSITION;
                is_up = false;
                break;
            }
            case ENGINE_WAIT_OBJECT: {
                (*di->print)("Wait object");
                sensor_d1 = (bool) (*si->get_value)(SENSOR_D1);
                if (sensor_d1 == true) {
                    state = ENGINE_BEGIN_WORK;
                }
                break;
            }
            case ENGINE_BEGIN_WORK: {
                (*di->print)("Begin work");
                sensor_d1 = (bool) (*si->get_value)(SENSOR_D1);
                sensor_d2 = (bool) (*si->get_value)(SENSOR_D2);
                sensor_d3 = (bool) (*si->get_value)(SENSOR_D3);
                sensor_d4 = (bool) (*si->get_value)(SENSOR_D4);
                sensor_d5 = (bool) (*si->get_value)(SENSOR_D5);
                if (sensor_d2 == true && sensor_d1 == false) {
                    (*di->do_step)(DEVICE_CATCH_LINE);
                    state = ENGINE_POSITION;
                    step_it = 0;
                    point_it = -1;
                }
                break;
            }
            case ENGINE_POSITION: {
                (*di->print)("Position");

                next_move_state = ENGINE_POSITION;
                state = ENGINE_MOVE_WELDING_DEVICE;
                welding_step_direct = DEVICE_WELDING_RIGHT;
                if (step_it >= center_step_count) {
                    state = ENGINE_FIND_NEXT_POINT;
                }
                break;
            }

            case ENGINE_FIND_NEXT_POINT: {
                (*di->print)("Find next point");
                if (point_it == 2) {
                    state = ENGINE_FREE;
                    break;
                }
                state = ENGINE_FIND_PROCESS;
                step_it = 0;
                point_it++;
                break;
            }

            case ENGINE_FREE: {
                (*di->print)("Free line");
                (*di->do_step)(DEVICE_FREE_LINE);
                state = ENGINE_WAIT_FREE_OBJECT;
                break;
            }

            case ENGINE_WAIT_FREE_OBJECT: {
                (*di->print)("Free object");
                sensor_d2 = (bool) (*si->get_value)(SENSOR_D2);
                if (sensor_d2 == false &&
                    sensor_d1 == false) {
                    state = ENGINE_WAIT_OBJECT;
                }
                break;
            }

            case ENGINE_FIND_PROCESS: {
                enum Property pointType;

                (*di->print)("Find point process");
                if (point_it == 0) {
                    pointType = VerticalStepsPoint1;
                } else if (point_it == 1) {
                    pointType = VerticalStepsPoint2;
                } else if (point_it == 2) {
                    pointType = VerticalStepsPoint3;
                } else {
                    state = ENGINE_ERROR;
                    break;
                }

                int v = (*pi->get_integer)(pointType);
                next_move_state = ENGINE_FIND_PROCESS;
                state = ENGINE_MOVE_WELDING_DEVICE;
                welding_step_direct = v > 0 ?
                                      DEVICE_WELDING_RIGHT :
                                      DEVICE_WELDING_LEFT;
                if (v == step_it) {
                    state = ENGINE_CONCRETE_NEXT_POINT_POSITION;
                }
                break;
            }

            case ENGINE_CONCRETE_NEXT_POINT_POSITION: {
                (*di->print)("Concrete point position");
                (*di->do_step)(DEVICE_POINT_POSITION);
                state = ENGINE_WELDING_DOWN;

                bool point_present = (bool)(*si->get_value)(SENSOR_POINT_PRESENT);
                if (point_present == false) {
                    state = ENGINE_POINT_NOT_FOUND;
                }
                break;
            }

            case ENGINE_POINT_NOT_FOUND: {
                (*di->print)("Point not found");
                (*di->do_step)(DEVICE_DROP_OBJECT);
                state = ENGINE_WAIT_OBJECT;
                break;
            }

            case ENGINE_WELDING_DOWN: {
                (*di->print)("Welding down");
                next_move_state = ENGINE_WELDING_DOWN;
                state = ENGINE_MOVE_WELDING_DEVICE;
                welding_step_direct = DEVICE_WELDING_DOWN;
                sensor_m4 = (bool) (*si->get_value)(SENSOR_M4);
                sensor_m3 = (bool) (*si->get_value)(SENSOR_M3);
                if (sensor_m4 == true && sensor_m3 == false) {
                    state = ENGINE_WELDING;
                }
                break;
            }

            case ENGINE_WELDING: {
                (*di->print)("Welding");
                (*di->do_step)(DEVICE_WELDING);
                state = ENGINE_WELDING_UP;
                break;
            }

            case ENGINE_WELDING_UP: {
                (*di->print)("Welding up");
                next_move_state = ENGINE_WELDING_UP;
                state = ENGINE_MOVE_WELDING_DEVICE;
                welding_step_direct = DEVICE_WELDING_UP;
                sensor_m4 = (bool) (*si->get_value)(SENSOR_M4);
                sensor_m3 = (bool) (*si->get_value)(SENSOR_M3);
                if (sensor_m4 == false && sensor_m3 == true) {
                    next_position_state = ENGINE_FIND_NEXT_POINT;
                    state = ENGINE_START_POSITION;
                    is_up = false;
                }
                break;
            }

            case ENGINE_START_POSITION: {
                (*di->print)("Start position");
                sensor_m1 = (bool) (*si->get_value)(SENSOR_M1);
                sensor_m2 = (bool) (*si->get_value)(SENSOR_M2);
                sensor_m3 = (bool) (*si->get_value)(SENSOR_M3);
                sensor_m4 = (bool) (*si->get_value)(SENSOR_M4);

                next_move_state = ENGINE_START_POSITION;
                state = ENGINE_MOVE_WELDING_DEVICE;
                if (sensor_m4 == false && sensor_m3 == true) {
                    is_up = true;
                }
                if (is_up == true &&
                           sensor_m1 == true && sensor_m2 == false) {
                    state = next_position_state;
                }

                if (is_up) {
                    welding_step_direct = DEVICE_WELDING_LEFT;
                } else {
                    welding_step_direct = DEVICE_WELDING_UP;
                }
                break;
            }

            case ENGINE_MOVE_WELDING_DEVICE: {
                (*di->print)("Move welding device");
                (*di->do_step)(welding_step_direct);
                ++step_it;
                state = next_move_state;
                break;
            }
            case ENGINE_ERROR: {
                (*di->print)("Exception");
                state = ENGINE_OFF;
                break;
            }
        }

        (*si->simulate_update)();
        /* Получаем данные о том выключено устройтсво или нет */
        if ((*si->get_value)(SENSOR_POWER_OFF)) {
            state = ENGINE_OFF;
        }
    }
}

