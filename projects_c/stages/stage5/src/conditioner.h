#ifndef _CONDITIONER_H_
#define _CONDITIONER_H_
#include <conditioner_i.h>

enum State {
    _PowerOff, _PowerOn, _Control, _Cooling, _Heating, _Waiting, _StorageError, _CondError
};

#if defined(__cplusplus)
extern "C" {
#endif

#define ERR_OK          0
#define ERR_STORAGE     1
#define ERR_CONDITIONER 2

int business_fsm(enum State state, struct ConditionerInter *cond, struct StorageInter *storage);

#if defined(__cplusplus)
}
#endif

#endif