#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
#include <stdbool.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum InsertingState {
    DELAY_STATE_ZERO, DELAY_STATE_ONE
};

enum DelayInputSymbol {
    DELAY_INPUT_ZERO = 0, DELAY_INPUT_ONE = 1
};

enum InsertingEvent {
    DELAY_OUTPUT_ZERO = 0, DELAY_OUTPUT_ONE = 1
};

struct InsertingEngine {
    enum InsertingState state;
};

#define DEFAULT_DELAY_STATE  DELAY_STATE_ZERO

bool delay_init(struct InsertingEngine *engine);

bool delay_reset(struct InsertingEngine *engine);

enum InsertingEvent delay_engine(struct InsertingEngine *engine, enum DelayInputSymbol symbol);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_DELAY_FSM_H
