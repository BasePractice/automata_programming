#include "inserting_fsm.h"

enum InsertingEvent inserting_engine(struct InsertingEngine *engine) {
    switch (engine->state) {
        case INSERTING_A:
            engine->s_1 = -1;
            engine->s_2 = -1;
            engine->state = INSERTING_B;
            return INSERTING_NEXT;
        case INSERTING_B:
            ++engine->s_1;
            ++engine->s_2;

            if (engine->s_2 >= engine->m_2_len && engine->s_1 < engine->m_1_len) {
                engine->state = INSERTING_A;
                return INSERTING_ERROR_END;
            } else if (engine->s_2 >= engine->m_2_len && engine->s_1 >= engine->m_1_len) {
                engine->state = INSERTING_A;
                return INSERTING_OK_END;
            }

            if (engine->m_1[engine->s_1] == engine->m_2[engine->s_2]) {
                return INSERTING_NEXT;
            } else if (engine->m_1[engine->s_1] != engine->m_2[engine->s_2]) {
                engine->state = INSERTING_C;
                engine->c_1 = engine->s_1;
                engine->c_2 = engine->s_2;
                return INSERTING_NEXT;
            }
            return INSERTING_ERROR_END;
        case INSERTING_C:
            ++engine->c_2;
            if (engine->c_2 >= engine->m_2_len && engine->s_1 < engine->m_1_len) {
                engine->state = INSERTING_A;
                return INSERTING_DETECT_END;
            } else if (engine->m_1[engine->c_1] == engine->m_2[engine->c_2]) {
                engine->state = INSERTING_B;
                engine->s_2 = engine->c_2;
                return INSERTING_DETECT;
            } else if (engine->m_1[engine->c_1] != engine->m_2[engine->c_2]) {
                return INSERTING_NEXT;
            }
            return INSERTING_ERROR_END;
    }
    return INSERTING_ERROR_END;
}

bool inserting_init(struct InsertingEngine *engine, char *m_1, size_t m_1_len, char *m_2, size_t m_2_len) {
    if (0 == engine || 0 == m_1 || 0 == m_2)
        return false;
    engine->state = INSERTING_A;
    engine->c_1 = -1;
    engine->c_2 = -1;

    engine->s_1 = -1;
    engine->s_2 = -1;

    engine->m_1 = m_1;
    engine->m_1_len = m_1_len;
    engine->m_2 = m_2;
    engine->m_2_len = m_2_len;
    return true;
}



