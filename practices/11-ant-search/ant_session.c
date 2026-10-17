#include "ant_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Тактов в истории: от старта до лимита включительно. */
#define HISTORY_SIZE (TRAIL_STEPS + 1)

struct AntSession *ant_session_create(void) {
    struct AntSession *s = (struct AntSession *)calloc(1, sizeof(*s));

    if (s == NULL) {
        return NULL;
    }
    s->history = (struct AntWalk *)malloc(sizeof(struct AntWalk) * HISTORY_SIZE);
    if (s->history == NULL) {
        free(s);
        return NULL;
    }
    s->steps_limit = TRAIL_STEPS;
    ant_walk_init(&s->history[0]);
    s->count = 1;
    return s;
}

void ant_session_destroy(struct AntSession *s) {
    if (s != NULL) {
        free(s->history);
        free(s);
    }
}

void ant_session_show(struct AntSession *s, const struct AntFsm *fsm, int steps_limit,
                      const char *title) {
    if (s->fsm.state_count > 0) {
        s->previous = s->fsm;
        s->has_previous = 1;
    }
    s->fsm = *fsm;
    s->steps_limit = (steps_limit < 1)             ? 1
                     : (steps_limit > TRAIL_STEPS) ? TRAIL_STEPS
                                                   : steps_limit;
    strncpy(s->title, (title != NULL) ? title : "", sizeof(s->title) - 1);
    s->title[sizeof(s->title) - 1] = '\0';
    ant_walk_init(&s->history[0]);
    s->count = 1;
}

void ant_session_forget_previous(struct AntSession *s) {
    s->has_previous = 0;
}

const struct AntWalk *ant_session_walk(const struct AntSession *s) {
    return &s->history[s->count - 1];
}

bool ant_session_done(const struct AntSession *s) {
    const struct AntWalk *w = ant_session_walk(s);
    return s->fsm.state_count <= 0 || w->run.finished || w->run.steps >= s->steps_limit;
}

bool ant_session_forward(struct AntSession *s) {
    if (ant_session_done(s) || s->count >= HISTORY_SIZE) {
        return false;
    }
    /* Такт делается на копии: прежний остаётся в истории для шага назад. */
    s->history[s->count] = s->history[s->count - 1];
    if (!ant_walk_step(&s->history[s->count], &s->fsm)) {
        return false;
    }
    ++s->count;
    return true;
}

bool ant_session_back(struct AntSession *s) {
    if (s->count <= 1) {
        return false;
    }
    --s->count;
    return true;
}

void ant_session_to_end(struct AntSession *s) {
    while (ant_session_forward(s)) {}
}

void ant_session_to_start(struct AntSession *s) {
    s->count = 1;
}

static enum AntAction random_action(struct AntRandom *r) {
    switch (ant_random_below(r, 3)) {
    case 0:
        return ANT_STEP;
    case 1:
        return ANT_TURN_LEFT;
    default:
        return ANT_TURN_RIGHT;
    }
}

void ant_fsm_random(struct AntFsm *fsm, int states, struct AntRandom *r) {
    int i;

    if (states < 1) {
        states = 1;
    }
    if (states > ANT_FSM_MAX_STATES) {
        states = ANT_FSM_MAX_STATES;
    }
    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = states;
    for (i = 0; i < states; ++i) {
        fsm->action[i][0] = random_action(r);
        fsm->action[i][1] = random_action(r);
        fsm->next[i][0] = ant_random_below(r, states);
        fsm->next[i][1] = ant_random_below(r, states);
    }
}

/* --- мнемоника и сравнение ------------------------------------------------ */

int ant_fsm_diff(const struct AntFsm *previous, const struct AntFsm *current, int state,
                 int input) {
    int diff = ANT_DIFF_SAME;

    if (state >= previous->state_count) {
        return ANT_DIFF_NEW;
    }
    if (previous->action[state][input] != current->action[state][input]) {
        diff |= ANT_DIFF_ACTION;
    }
    if (previous->next[state][input] != current->next[state][input]) {
        diff |= ANT_DIFF_NEXT;
    }
    return diff;
}

int ant_fsm_diff_count(const struct AntFsm *previous, const struct AntFsm *current) {
    int state;
    int input;
    int count = 0;

    for (state = 0; state < current->state_count; ++state) {
        for (input = 0; input < 2; ++input) {
            if (ant_fsm_diff(previous, current, state, input) != ANT_DIFF_SAME) {
                ++count;
            }
        }
    }
    return count;
}

void ant_transition_text(const struct AntFsm *fsm, int state, int input, char *out) {
    const char *letter;

    switch (fsm->action[state][input]) {
    case ANT_STEP:
        letter = "Ш";
        break;
    case ANT_TURN_LEFT:
        letter = "Л";
        break;
    default:
        letter = "П";
        break;
    }
    sprintf(out, "%s%d", letter, fsm->next[state][input]);
}

/* --- каталог ------------------------------------------------------------- */

#define HAND "Придуман вручную"
#define SEARCH "Журнал поиска от scan"
#define FOUND "Найден поиском"

/*
 * Записи журнала — прогон этой же практики:
 *
 *     11-ant-search --from scan --states 20 --grow 3000 \
 *                   --iterations 2000000 --seed 1
 *
 * Каждая запись строго лучше предыдущей; вместе они показывают, как
 * автомат обрастает состояниями (5 → 6 → … → 20), сокращая путь. Числа в
 * названиях проверяются тестом прогоном, а не принимаются на слово.
 */
/* clang-format off */
static const struct AntPreset PRESETS[] = {
    {"Осмотр: 4 состояния", HAND, "смотрит вправо и влево; встаёт на первом разрыве в 2 клетки",
     "lookaround", TRAIL_STEPS, 42, TRAIL_STEPS},
    {"Проба: 6 состояний", HAND, "два шага вслепую; встаёт на диагональном повороте",
     "probe", TRAIL_STEPS, 59, TRAIL_STEPS},
    {"Сканер: начальный, 315 тактов", HAND,
     "осмотр четырёх сторон и шаг вслепую; с него начат поиск", "scan", TRAIL_STEPS, 89, 315},
    {"Поиск, запись 1: 295 тактов", SEARCH, "5 состояний",
     "3.0.0.1:3.0.0.2:0.1.0.3:3.4.0.4:3.0.2.0", TRAIL_STEPS, 89, 295},
    {"Поиск, запись 2: 283 такта", SEARCH, "5 состояний",
     "3.0.0.1:3.0.0.2:0.2.0.3:3.4.0.4:3.4.2.0", TRAIL_STEPS, 89, 283},
    {"Поиск, запись 3: 275 тактов", SEARCH, "5 состояний",
     "3.0.0.1:3.4.0.2:1.4.0.3:3.4.0.4:3.0.2.0", TRAIL_STEPS, 89, 275},
    {"Поиск, запись 4: 235 тактов", SEARCH, "6 состояний",
     "3.0.0.1:3.4.0.2:1.1.0.3:3.4.0.5:3.0.2.5:3.4.2.0", TRAIL_STEPS, 89, 235},
    {"Поиск, запись 5: 233 такта", SEARCH, "6 состояний",
     "3.0.0.1:3.4.1.2:3.0.1.3:3.4.0.5:3.5.2.5:3.2.2.0", TRAIL_STEPS, 89, 233},
    {"Поиск, запись 6: 229 тактов", SEARCH, "7 состояний",
     "3.6.0.1:3.4.1.2:3.0.1.3:3.4.0.5:3.5.2.5:3.2.2.0:3.2.0.1", TRAIL_STEPS, 89, 229},
    {"Поиск, запись 7: 213 тактов", SEARCH, "7 состояний",
     "3.6.0.1:3.4.1.2:3.5.1.3:3.4.0.5:3.2.2.5:3.6.2.0:3.2.0.1", TRAIL_STEPS, 89, 213},
    {"Поиск, запись 8: 211 тактов", SEARCH, "8 состояний",
     "3.7.0.2:3.0.2.0:3.4.1.6:3.4.0.1:3.6.2.1:3.7.2.7:3.5.1.3:3.6.0.2", TRAIL_STEPS, 89, 211},
    {"Поиск, запись 9: 201 такт", SEARCH, "8 состояний",
     "3.7.0.2:3.4.2.0:3.4.1.6:3.4.0.1:3.6.2.1:3.7.2.6:3.5.1.3:3.6.0.2", TRAIL_STEPS, 89, 201},
    {"Поиск, запись 10: 193 такта", SEARCH, "8 состояний",
     "3.7.0.2:3.5.2.0:3.4.1.6:3.4.0.1:3.6.2.1:3.7.2.1:3.5.1.3:3.6.0.2", TRAIL_STEPS, 89, 193},
    {"Поиск, запись 11: 183 такта", SEARCH, "11 состояний",
     "3.7.0.2:3.7.2.5:3.4.1.8:0.1.2.8:3.8.2.3:3.1.2.0:3.2.1.2:3.8.0.2:3.1.1.9"
     ":3.10.0.5:3.6.2.5", TRAIL_STEPS, 89, 183},
    {"Поиск, запись 12: 181 такт", SEARCH, "11 состояний",
     "3.7.0.2:3.7.2.5:3.4.1.8:0.8.2.8:3.8.2.3:3.1.2.0:3.4.1.5:3.8.0.6:3.1.1.9"
     ":3.10.0.5:3.5.2.5", TRAIL_STEPS, 89, 181},
    {"Поиск, запись 13: 179 тактов", SEARCH, "11 состояний",
     "3.7.0.2:3.7.2.5:3.4.1.8:0.9.2.8:3.5.2.3:3.1.2.0:3.4.1.3:3.8.0.6:3.1.1.9"
     ":3.10.0.5:3.8.2.5", TRAIL_STEPS, 89, 179},
    {"Поиск, запись 14: 175 тактов", SEARCH, "11 состояний",
     "3.7.0.2:3.7.2.5:3.4.1.8:3.3.2.8:3.3.2.3:3.9.2.0:3.7.1.3:3.8.0.6:3.1.1.9"
     ":3.10.0.5:3.8.2.5", TRAIL_STEPS, 89, 175},
    {"Поиск, запись 15: 173 такта", SEARCH, "18 состояний",
     "3.12.0.2:3.12.2.7:3.14.1.8:3.3.2.8:3.14.0.13:3.5.1.6:3.12.1.3:3.3.2.0"
     ":3.1.1.9:3.10.0.7:3.15.2.7:3.5.0.5:3.8.0.6:0.2.1.6:3.7.2.3:3.2.1.14"
     ":3.0.2.1:3.14.0.3", TRAIL_STEPS, 89, 173},
    {"Поиск, запись 16: 171 такт", SEARCH, "18 состояний",
     "3.12.0.2:3.12.2.4:3.14.1.15:3.16.1.9:0.16.2.0:3.9.2.14:3.17.0.10:3.11.1.0"
     ":3.1.1.9:3.10.0.4:3.15.2.4:3.3.2.4:3.8.0.1:0.9.0.15:3.17.2.16:3.2.1.9"
     ":3.16.2.8:3.11.2.0", TRAIL_STEPS, 89, 171},
    {"Поиск, запись 17: 167 тактов", SEARCH, "20 состояний",
     "3.12.0.2:3.12.2.4:3.14.1.8:3.6.2.0:1.3.2.0:3.17.2.19:3.14.0.4:3.9.2.19"
     ":3.1.1.9:3.10.0.3:3.3.2.4:3.5.2.3:3.8.0.1:3.14.2.5:3.17.2.5:1.2.2.15"
     ":1.14.0.3:3.11.2.16:1.10.1.14:3.15.1.6", TRAIL_STEPS, 89, 167},
    {"Поиск, запись 18: 163 такта", SEARCH, "20 состояний",
     "3.12.0.2:3.12.2.3:3.11.1.8:3.11.2.0:0.6.0.8:3.17.2.8:3.14.0.3:0.16.1.10"
     ":3.1.1.9:3.10.0.3:3.17.2.3:3.5.2.16:3.8.0.1:0.2.1.12:3.10.2.1:0.15.1.10"
     ":1.1.2.19:3.11.2.6:0.13.1.10:3.12.1.6", TRAIL_STEPS, 89, 163},
    {"Поиск, запись 19: 161 такт", SEARCH, "20 состояний",
     "3.12.0.2:3.12.2.3:3.11.1.8:3.14.2.0:1.9.0.14:3.17.2.8:3.14.0.3:3.11.0.7"
     ":3.1.1.9:3.10.0.17:3.17.2.3:3.5.2.16:3.8.0.1:0.7.1.10:3.5.2.1:3.15.2.19"
     ":0.11.2.19:3.11.2.6:1.13.0.2:3.12.1.6", TRAIL_STEPS, 89, 161},
    {"Поиск, запись 20: 157 тактов", SEARCH, "20 состояний",
     "3.10.1.9:3.12.2.18:3.11.1.0:3.14.2.15:3.18.0.2:3.17.2.8:3.14.0.3:1.3.2.11"
     ":3.1.1.11:3.1.0.17:3.9.1.11:3.5.2.16:3.8.0.19:3.14.2.18:3.5.2.1:3.18.1.8"
     ":3.1.2.19:3.11.2.6:1.8.2.4:3.12.1.6", TRAIL_STEPS, 89, 157},
    {"Поиск, запись 21: 155 тактов", SEARCH, "20 состояний",
     "3.12.2.6:3.12.2.15:0.14.1.6:3.7.0.13:3.14.2.15:1.11.2.19:3.14.0.19"
     ":3.17.2.8:3.1.1.14:3.4.0.17:3.5.1.9:3.6.2.16:3.8.0.1:3.11.1.18:3.7.2.4"
     ":3.12.2.3:1.16.2.2:3.14.2.9:3.11.1.10:3.7.2.10", TRAIL_STEPS, 89, 155},
    {"Поиск, запись 22: 153 такта", SEARCH, "20 состояний",
     "3.5.2.3:3.12.2.0:0.2.1.6:3.12.0.13:0.13.2.1:3.9.2.10:3.4.0.19:3.17.2.8"
     ":3.1.1.18:3.1.0.17:1.10.2.17:3.14.2.15:3.8.0.0:3.11.1.18:3.7.2.1:3.5.2.2"
     ":3.18.1.9:3.14.2.9:3.13.1.15:3.8.2.16", TRAIL_STEPS, 89, 153},
    {"Найден в c_fsm: 181 такт", FOUND, "17 состояний, журнал поиска из репозитория c_fsm",
     "evolved", TRAIL_STEPS, 89, 181},
    {"С нуля, 5 состояний: 275 тактов", FOUND,
     "11-ant-search --states 5 --seed 1: другой путь, чем у scan",
     "3.2.2.2:0.4.0.3:3.2.0.4:3.0.0.0:3.0.0.1", TRAIL_STEPS, 89, 275}
};
/* clang-format on */

#define PRESET_COUNT ((int)(sizeof(PRESETS) / sizeof(PRESETS[0])))

const struct AntPreset *ant_preset(int index) {
    return (index >= 0 && index < PRESET_COUNT) ? &PRESETS[index] : NULL;
}

int ant_preset_count(void) {
    return PRESET_COUNT;
}

bool ant_preset_fsm(const struct AntPreset *preset, struct AntFsm *fsm) {
    return ant_fsm_by_name(preset->spec, fsm) || ant_fsm_parse(fsm, preset->spec);
}

/* --- режимы поиска --------------------------------------------------------- */

/* clang-format off */
static const struct AntSearchMode MODES[] = {
    {"(1+1)-ЭС от автомата на экране", "мутации текущего автомата, рост до 20 состояний",
     ANT_SEARCH_HILL, ANT_START_CURRENT, 20, 3000L, 2000000L, 1},
    {"(1+1)-ЭС от scan", "та же постановка, что дала журнал каталога (зерно своё)",
     ANT_SEARCH_HILL, ANT_START_SCAN, 20, 3000L, 2000000L, 1},
    {"(1+1)-ЭС с нуля, 5 состояний", "случайный автомат и мутации без роста",
     ANT_SEARCH_HILL, ANT_START_RANDOM, 5, 0L, 200000L, 1},
    {"(1+1)-ЭС с нуля, 12 состояний", "больше состояний — больше свободы и медленнее",
     ANT_SEARCH_HILL, ANT_START_RANDOM, 12, 0L, 500000L, 1},
    {"EP93: постановка статьи", "популяция 300, лимит 200 тактов, заморозка модулей",
     ANT_SEARCH_EP93, ANT_START_RANDOM, ANT_FSM_MAX_STATES, 0L, 1000000L, 1},
    {"EP93 без заморозки", "та же постановка без оператора сжатия",
     ANT_SEARCH_EP93, ANT_START_RANDOM, ANT_FSM_MAX_STATES, 0L, 1000000L, 0}
};
/* clang-format on */

#define MODE_COUNT ((int)(sizeof(MODES) / sizeof(MODES[0])))

const struct AntSearchMode *ant_search_mode(int index) {
    return (index >= 0 && index < MODE_COUNT) ? &MODES[index] : NULL;
}

int ant_search_mode_count(void) {
    return MODE_COUNT;
}

void ant_search_mode_config(const struct AntSearchMode *mode, uint32_t seed,
                            const struct AntFsm *current, struct AntSearch *cfg,
                            struct AntFsm *start, int *has_start) {
    if (mode->algorithm == ANT_SEARCH_EP93) {
        ant_search_defaults_ep93(cfg);
    } else {
        ant_search_defaults(cfg);
    }
    cfg->max_states = mode->max_states;
    cfg->stall_limit = mode->stall_limit;
    cfg->iterations = mode->iterations;
    cfg->compression = mode->compression ? true : false;
    cfg->seed = seed;

    *has_start = 1;
    switch (mode->start) {
    case ANT_START_SCAN:
        ant_fsm_scan(start);
        break;
    case ANT_START_CURRENT:
        if (current != NULL && current->state_count > 0) {
            *start = *current;
            if (cfg->max_states < current->state_count) {
                cfg->max_states = current->state_count;
            }
        } else {
            ant_fsm_scan(start);
        }
        break;
    default:
        *has_start = 0;
        break;
    }
}
