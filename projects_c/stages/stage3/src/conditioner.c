#include "conditioner.h"

int
business_fsm(enum State state, struct ConditionerInter *cond, struct StorageInter *storage) {
    int ret = 1;
    do {
        /** Для тестирования. Эмуляция внешнего управления*/
        if (cond->is_running && !cond->is_running(state)) {
            state = _PowerOff;
        }
        switch (state) {
            case _PowerOff: {
                /** Не хватает обработки ошибок */
                ret = storage->property_store();
                if ( (*storage->base.has_errors)() ) {
                    state = _Error;
                }
                break;
            }
            case _PowerOn: {
                /** Не хватает обработки ошибок */
                storage->property_load();
                state = _Control;
                break;
            }
            case _Waiting: {
                int wait = storage->property_get_int(WaitingIdle);
                /** Не хватает обработки ошибок */
                cond->process_engine(Waiting, wait);
                state = _Control;
                break;
            }
            case _Heating: {
                int wait = storage->property_get_int(WaitingHeating);
                /** Не хватает обработки ошибок */
                cond->process_engine(Heating, wait);
                state = _Control;
                break;
            }
            case _Cooling: {
                int wait = storage->property_get_int(WaitingCooling);
                /** Не хватает обработки ошибок */
                cond->process_engine(Cooling, wait);
                state = _Control;
                break;
            }
            case _Control: {
                int guard_temperature = storage->property_get_int(Temperature);
                int current_temperature = cond->get_current_temperature();
                if (current_temperature > guard_temperature) {
                    state = _Cooling;
                } else if (current_temperature < guard_temperature) {
                    state = _Heating;
                } else {
                    state = _Waiting;
                }
                break;
            }
            case _Error: {
                state = _PowerOff;
                break;
            }
        }
    } while (state != _PowerOff);
    return ret;
}