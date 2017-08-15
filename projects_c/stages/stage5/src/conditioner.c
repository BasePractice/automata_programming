#include "conditioner.h"

static enum State
check_errors(enum State state, struct ConditionerInter *cond, struct StorageInter *storage) {
    if ((*storage->base.errors)()) {
        return _StorageError;
    }
    if ((*cond->base.errors)()) {
        return _CondError;
    }
    return state;
}

int
business_fsm(enum State state, struct ConditionerInter *cond, struct StorageInter *storage) {
    int ret = ERR_OK;
    do {
        state = (enum State)cond->control(state);
        switch (state) {
            case _PowerOff: {
                storage->property_store();
                state = check_errors(state, cond, storage);
                break;
            }
            case _PowerOn: {
                storage->property_load();
                state = check_errors(_Control, cond, storage);
                break;
            }
            case _Waiting: {
                int wait = storage->property_get_int(WaitingIdle);
                cond->process_engine(Waiting, wait);
                state = check_errors(_Control, cond, storage);
                break;
            }
            case _Heating: {
                int wait = storage->property_get_int(WaitingHeating);
                cond->process_engine(Heating, wait);
                state = check_errors(_Control, cond, storage);
                break;
            }
            case _Cooling: {
                int wait = storage->property_get_int(WaitingCooling);
                cond->process_engine(Cooling, wait);
                state = check_errors(_Control, cond, storage);
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
                state = check_errors(state, cond, storage);
                break;
            }
            case _StorageError: {
                /** Обработка ошибок */
                state = _PowerOff;
                ret = ERR_STORAGE;
                break;
            }
            case _CondError: {
                /** Обработка ошибок */
                state = _PowerOff;
                ret = ERR_CONDITIONER;
                break;
            }
        }
    } while (state != _PowerOff);
    return ret;
}