#ifndef C_AUTOMATA_PROGRAMMING_PRACTICE_INSERTING_FSM_H
#define C_AUTOMATA_PROGRAMMING_PRACTICE_INSERTING_FSM_H
#include <stdbool.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

enum InsertingState {
    INSERTING_A,
    INSERTING_B,
    INSERTING_C
};

enum InsertingEvent {
    INSERTING_ERROR_END, INSERTING_OK_END, INSERTING_DETECT, INSERTING_DETECT_END, INSERTING_NEXT
};

struct InsertingEngine {
    enum InsertingState state;
    bool            running;
    size_t          s_1;
    size_t          s_2;

    size_t          c_1;
    size_t          c_2;

    char           *m_1;
    size_t         m_1_len;
    char           *m_2;
    size_t         m_2_len;
};

bool inserting_init(struct InsertingEngine *engine, char *m_1, size_t m_1_len, char *m_2, size_t m_2_len);

enum InsertingEvent inserting_engine(struct InsertingEngine *engine);

#if defined(__cplusplus)
}
#endif

#endif //C_AUTOMATA_PROGRAMMING_PRACTICE_INSERTING_FSM_H
