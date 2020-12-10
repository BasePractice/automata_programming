#include <stdio.h>
#include <limits.h>
#include "pneumo_ctrl.h"

#if defined(PNEUMO_SIMULATE)
#define TIMEOUT_DELTA(timeout)  1
#define DELAY_DELTA(delay)      1
#else
#define TIMEOUT_DELTA(timeout)  ((timeout) * 1000)
#define DELAY_DELTA(delay)      ((delay) * 1)
#endif

#if defined(PNEUMO_DEBUG)
static char *state_names[] = {
        "PneumoState_I", "PneumoState_1", "PneumoState_2", "PneumoState_3",
        "PneumoState_4", "PneumoState_5", "PneumoState_6", "PneumoState_7",
        "PneumoState_8", "PneumoState_9", "PneumoState_E",
};
#endif

void pneumo_engine_init(struct PneumoEngine *engine) {
    if (0 != engine) {
        engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP] = 0;
        engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN] = 0;
        engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;

        engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_UP] = 0;
        engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN] = 0;
        engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;

        engine->state = PneumoState_Init;
        engine->delay = 0;
        engine->timeout = 0;
        engine->timeouts[PneumoState_Init] = INT_MAX;
        engine->delays[PneumoState_Init] = INT_MAX;
        engine->timeouts[PneumoState_1] = TIMEOUT_DELTA(120);
        engine->delays[PneumoState_1] = DELAY_DELTA(10);
        engine->timeouts[PneumoState_2] = TIMEOUT_DELTA(120);
        engine->delays[PneumoState_2] = DELAY_DELTA(60);
        engine->timeouts[PneumoState_3] = TIMEOUT_DELTA(120);
        engine->delays[PneumoState_3] = DELAY_DELTA(60);
        engine->timeouts[PneumoState_4] = TIMEOUT_DELTA(120);
        engine->delays[PneumoState_4] = DELAY_DELTA(10);
        engine->timeouts[PneumoState_5] = TIMEOUT_DELTA(200);
        engine->delays[PneumoState_5] = DELAY_DELTA(60);
        engine->timeouts[PneumoState_6] = TIMEOUT_DELTA(120);
        engine->delays[PneumoState_6] = DELAY_DELTA(60);
        engine->timeouts[PneumoState_7] = TIMEOUT_DELTA(200);
        engine->delays[PneumoState_7] = DELAY_DELTA(60);
        engine->timeouts[PneumoState_8] = TIMEOUT_DELTA(120);
        engine->delays[PneumoState_8] = DELAY_DELTA(60);
        engine->timeouts[PneumoState_9] = TIMEOUT_DELTA(200);
        engine->delays[PneumoState_9] = DELAY_DELTA(10);
    }
}

#define TIMEOUT_GE(engine) ( (engine)->timeout > (engine)->timeouts[(engine)->state] )
#define DELAY_GE(engine) ( (engine)->delay > (engine)->delays[(engine)->state] )

bool pneumo_engine_tick(struct PneumoEngine *engine) {
    bool ret = true;
    if (0 == engine)
        return false;
#if defined(PNEUMO_DEBUG)
    fprintf(stdout, "State: %s, Y1(in): [%d, %d], Y2(in): [%d, %d], Y1(out): [%d], Y2(out): [%d]\n",
            state_names[engine->state],
            engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN],
            engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP],
            engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN],
            engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_UP],
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal,
            engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal
    );
    fflush(stdout);
#endif
    switch (engine->state) {
        case PneumoState_Init: {
            engine->state = PneumoState_1;
            engine->delay = 0;
            engine->timeout = 0;
            break;
        }
        case PneumoState_1: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
            engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN] &&
                engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_2;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_6;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_2: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 1;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_3;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_3: {
            engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 1;
            if (engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_UP]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_4;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_4: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_5;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_5: {
            engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
            if (engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_6;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_6: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 1;
            engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 1;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP] &&
                engine->cylinders[PNEUMO_CYLINDER_Y2].input_signal[PNEUMO_CYLINDER_SIGNAL_UP]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_7;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_7: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_8;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_8: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 1;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_UP]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_9;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_9: {
            engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
            if (engine->cylinders[PNEUMO_CYLINDER_Y1].input_signal[PNEUMO_CYLINDER_SIGNAL_DOWN]) {
                engine->timeout = 0;
                if (DELAY_GE(engine)) {
                    engine->state = PneumoState_1;
                    engine->delay = 0;
                    engine->timeout = 0;
                }
            } else if (TIMEOUT_GE(engine)) {
                engine->state = PneumoState_FatalException;
                engine->cylinders[PNEUMO_CYLINDER_Y1].output_signal = 0;
                engine->cylinders[PNEUMO_CYLINDER_Y2].output_signal = 0;
                engine->delay = 0;
                engine->timeout = 0;
            }
            break;
        }
        case PneumoState_FatalException: {
            ret = false;
            break;
        }
    }
    engine->timeout++;
    engine->delay++;
    return ret;
}

void pneumo_engine_destroy(struct PneumoEngine *engine) {
    if (0 != engine) {
        //Освобождаем ресурсы, если есть
    }
}