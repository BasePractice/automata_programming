#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum DelayState {
    DELAY_STATE_ZERO, DELAY_STATE_ONE
};

enum DelayInputSymbol {
    DELAY_INPUT_ZERO = 0, DELAY_INPUT_ONE = 1
};

enum DelayOutputSymbol {
    DELAY_OUTPUT_ZERO = 0, DELAY_OUTPUT_ONE = 1
};

struct DelayEngine {
    enum DelayState state;
};

#define DEFAULT_DELAY_STATE  DELAY_STATE_ZERO

bool delay_init(struct DelayEngine *engine);

bool delay_reset(struct DelayEngine *engine);

enum DelayOutputSymbol delay_engine(struct DelayEngine *engine, enum DelayInputSymbol symbol);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
