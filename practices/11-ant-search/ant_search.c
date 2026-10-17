/*
 * Поиск автомата для задачи об умном муравье.
 *
 * Здесь — общая часть (приспособленность, проверка настроек, выбор алгоритма)
 * и простейший алгоритм, (1 + 1)-эволюционная стратегия:
 *
 *   current — автомат, вокруг которого идёт поиск;
 *   best    — лучший из встреченных за всё время;
 *
 *   на каждой итерации copy = mutate(current); если прогон copy лучше
 *   прогона current — copy становится current; если он лучше и best —
 *   обновляется и best, и вызывается журнал улучшений.
 *
 * Равные по качеству мутации тоже принимаются (нейтральный дрейф): без этого
 * поиск застревает на первом же плато, а плато здесь огромны — большинство
 * перестановок в таблице переходов не меняют поведения на тропе вовсе.
 *
 * Популяционный алгоритм из статьи Angeline и Pollack лежит в ant_ep93.c.
 */

#include "ant_search.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "ant_ep93.h"
#include "ant_random.h"

/* --- функция приспособленности --------------------------------------------- */

bool ant_run_better(const struct TrailRun *a, const struct TrailRun *b) {
    if (a->eaten != b->eaten) {
        return a->eaten > b->eaten;
    }
    return a->steps < b->steps;
}

double ant_fitness_ep93(const struct TrailRun *run, int steps_limit) {
    double t;

    if (steps_limit < 1) {
        return (double)run->eaten;
    }
    t = (double)run->last_eaten / (double)steps_limit;
    return (double)run->eaten + 0.01 * (1.0 - t);
}

static bool run_equal(const struct TrailRun *a, const struct TrailRun *b) {
    return a->eaten == b->eaten && a->steps == b->steps;
}

/* --- мутации ---------------------------------------------------------------- */

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

/* Случайная таблица переходов заданного размера. */
static void fsm_random(struct AntFsm *fsm, int state_count, struct AntRandom *r) {
    int i;

    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = state_count;
    for (i = 0; i < state_count; ++i) {
        fsm->action[i][0] = random_action(r);
        fsm->action[i][1] = random_action(r);
        fsm->next[i][0] = ant_random_below(r, state_count);
        fsm->next[i][1] = ant_random_below(r, state_count);
    }
}

/*
 * Точечная мутация: одно из четырёх полей одного состояния заменяется
 * случайным значением. Поля: действие и переход для входа «еда впереди» и то
 * же самое для входа «пусто».
 */
static void fsm_mutate_once(struct AntFsm *fsm, struct AntRandom *r) {
    int state = ant_random_below(r, fsm->state_count);
    int input = ant_random_below(r, 2);

    if (ant_random_below(r, 2) == 0) {
        fsm->action[state][input] = random_action(r);
    } else {
        fsm->next[state][input] = ant_random_below(r, fsm->state_count);
    }
}

static void fsm_mutate(struct AntFsm *fsm, struct AntRandom *r) {
    fsm_mutate_once(fsm, r);
    /* Изредка — сразу две правки: одиночной мутации иногда не хватает, чтобы
       сойти с плато, потому что действие и переход связаны по смыслу. */
    if (ant_random_below(r, 3) == 0) {
        fsm_mutate_once(fsm, r);
    }
}

/*
 * Рост автомата: добавленное состояние заполняется случайно, и на него
 * заводится ровно один переход из уже существующего состояния — иначе новое
 * состояние недостижимо и поиск потратит итерации впустую.
 */
static void fsm_grow(struct AntFsm *fsm, struct AntRandom *r) {
    int added = fsm->state_count;
    int from;
    int input;

    fsm->action[added][0] = random_action(r);
    fsm->action[added][1] = random_action(r);
    fsm->next[added][0] = ant_random_below(r, added + 1);
    fsm->next[added][1] = ant_random_below(r, added + 1);
    fsm->state_count = added + 1;

    from = ant_random_below(r, added);
    input = ant_random_below(r, 2);
    fsm->next[from][input] = added;
}

/* --- настройки --------------------------------------------------------------- */

void ant_search_defaults(struct AntSearch *cfg) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->algorithm = ANT_SEARCH_HILL;
    cfg->max_states = 5;
    cfg->steps_limit = TRAIL_STEPS;
    cfg->iterations = 200000L;
    cfg->stall_limit = 0;
    cfg->seed = 1;
    /* Значения из статьи: пригодятся, если алгоритм переключат на EP93. */
    cfg->population = 300;
    cfg->tournaments = 5;
    cfg->compression = true;
}

void ant_search_defaults_ep93(struct AntSearch *cfg) {
    ant_search_defaults(cfg);
    cfg->algorithm = ANT_SEARCH_EP93;
    cfg->max_states = ANT_FSM_MAX_STATES;
    cfg->steps_limit = TRAIL_STEPS_EP93;
    cfg->iterations = 1000000L;
}

/* --- (1 + 1)-эволюционная стратегия ------------------------------------------ */

/*
 * Всё, что поиск помнит между вызовами ant_search_advance. У (1 + 1)-ЭС это
 * текущий автомат, датчик и счётчики; у EP93 — популяция (ant_ep93.c).
 */
struct AntSearchState {
    struct AntSearch cfg;
    struct Ep93State *ep93; /* не NULL — идёт поиск по статье */
    struct AntRandom random;
    struct AntFsm current;
    struct TrailRun current_run;
    struct AntSearchResult result;
    long stall;
};

static void hill_begin(struct AntSearchState *s, const struct AntFsm *start) {
    ant_random_init(&s->random, s->cfg.seed);
    if (start != NULL) {
        s->current = *start;
    } else {
        fsm_random(&s->current, s->cfg.max_states, &s->random);
    }
    s->current_run = ant_trail_run(&s->current, s->cfg.steps_limit);

    memset(&s->result, 0, sizeof(s->result));
    s->result.best = s->current;
    s->result.run = s->current_run;
    s->result.evaluations = 1;
    s->stall = 0;
}

/* Одна итерация: мутант текущего автомата и сравнение с ним. */
static void hill_iteration(struct AntSearchState *s, const struct AntSearchLog *log) {
    struct AntFsm candidate = s->current;
    struct TrailRun candidate_run;
    struct AntSearchResult *result = &s->result;

    if (s->cfg.stall_limit > 0 && s->stall >= s->cfg.stall_limit
        && candidate.state_count < s->cfg.max_states) {
        fsm_grow(&candidate, &s->random);
        s->stall = 0;
    } else {
        fsm_mutate(&candidate, &s->random);
    }
    candidate_run = ant_trail_run(&candidate, s->cfg.steps_limit);
    ++result->iterations_done;
    ++result->evaluations;
    if (candidate_run.finished && result->solved_at == 0) {
        result->solved_at = result->evaluations;
    }

    if (ant_run_better(&candidate_run, &s->current_run)
        || run_equal(&candidate_run, &s->current_run)) {
        s->current = candidate;
        s->current_run = candidate_run;
    }
    if (ant_run_better(&candidate_run, &result->run)) {
        result->best = candidate;
        result->run = candidate_run;
        ++result->improvements;
        s->stall = 0;
        if (log != NULL && log->improved != NULL) {
            (*log->improved)(&result->best, &result->run, result->iterations_done, log->user);
        }
    } else {
        ++s->stall;
    }
}

/*
 * Поиск не останавливается, когда тропа впервые пройдена: дальше он
 * сокращает число тактов, и именно так получена история улучшений
 * 315 → 295 → … → 181 из лекции.
 */
static bool hill_advance(struct AntSearchState *s, long budget, const struct AntSearchLog *log) {
    while (budget > 0 && s->result.iterations_done < s->cfg.iterations) {
        hill_iteration(s, log);
        --budget;
    }
    return s->result.iterations_done >= s->cfg.iterations;
}

/* --- выбор алгоритма ---------------------------------------------------------- */

static bool config_valid(const struct AntSearch *cfg, const struct AntFsm *start) {
    if (cfg == NULL) {
        return false;
    }
    if (cfg->max_states < 1 || cfg->max_states > ANT_FSM_MAX_STATES) {
        return false;
    }
    if (cfg->steps_limit < 1 || cfg->iterations < 0 || cfg->stall_limit < 0) {
        return false;
    }
    if (start != NULL && (start->state_count < 1 || start->state_count > cfg->max_states)) {
        return false;
    }
    return true;
}

struct AntSearchState *ant_search_begin(const struct AntSearch *cfg, const struct AntFsm *start,
                                        const struct AntSearchLog *log) {
    struct AntSearchState *s;

    if (!config_valid(cfg, start)) {
        return NULL;
    }
    s = (struct AntSearchState *)calloc(1, sizeof(*s));
    if (s == NULL) {
        return NULL;
    }
    s->cfg = *cfg;
    if (cfg->algorithm == ANT_SEARCH_EP93) {
        s->ep93 = ant_ep93_begin(cfg, start, log);
        if (s->ep93 == NULL) {
            free(s);
            return NULL;
        }
    } else {
        hill_begin(s, start);
    }
    return s;
}

bool ant_search_advance(struct AntSearchState *s, long budget, const struct AntSearchLog *log) {
    if (s->ep93 != NULL) {
        return ant_ep93_advance(s->ep93, budget, log);
    }
    return hill_advance(s, budget, log);
}

const struct AntSearchResult *ant_search_result(const struct AntSearchState *s) {
    return (s->ep93 != NULL) ? ant_ep93_result(s->ep93) : &s->result;
}

const struct AntSearch *ant_search_config(const struct AntSearchState *s) {
    return &s->cfg;
}

void ant_search_end(struct AntSearchState *s) {
    if (s != NULL) {
        ant_ep93_end(s->ep93);
        free(s);
    }
}

bool ant_search_run(const struct AntSearch *cfg, const struct AntFsm *start,
                    struct AntSearchResult *out, const struct AntSearchLog *log) {
    struct AntSearchState *s;

    if (out == NULL) {
        return false;
    }
    s = ant_search_begin(cfg, start, log);
    if (s == NULL) {
        return false;
    }
    (void)ant_search_advance(s, LONG_MAX, log);
    *out = *ant_search_result(s);
    ant_search_end(s);
    return true;
}
