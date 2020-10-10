#include "delay_fsm.h"

enum DelayOutputSymbol delay_engine(struct DelayEngine *engine, enum DelayInputSymbol symbol) {
    switch (engine->state) {
        case DELAY_STATE_ZERO: {
            if (DELAY_INPUT_ONE == symbol) {
                engine->state = DELAY_STATE_ONE;
                return DELAY_OUTPUT_ZERO;
            } else if (DELAY_INPUT_ZERO == symbol) {
                return DELAY_OUTPUT_ZERO;
            }
            break;
        }
        case DELAY_STATE_ONE: {
            if (DELAY_INPUT_ONE == symbol) {
                return DELAY_OUTPUT_ONE;
            } else if (DELAY_INPUT_ZERO == symbol) {
                engine->state = DELAY_STATE_ZERO;
                return DELAY_OUTPUT_ONE;
            }
            break;
        }
    }
    return DELAY_OUTPUT_ZERO;
}

bool delay_init(struct DelayEngine *engine) {
    if (0 == engine)
        return false;
    engine->state = DEFAULT_DELAY_STATE;
    return true;
}

bool delay_reset(struct DelayEngine *engine) {
    return delay_init(engine);
}


