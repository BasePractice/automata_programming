#ifndef _CONDITIONER_H_
#define _CONDITIONER_H_
#include <conditioner_i.h>

enum State {
    _PowerOff, _PowerOn, _Control, _Cooling, _Heating, _Waiting
};

#if defined(__cplusplus)
extern "C" {
#endif

int business_fsm(enum State state, struct ConditionerInter *cond, struct StorageInter *storage);

#if defined(__cplusplus)
}
#endif

#endif