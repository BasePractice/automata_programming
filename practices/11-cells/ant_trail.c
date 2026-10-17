/*
 * Каноническая тропа Санта-Фе и прогон автомата муравья по ней.
 *
 * Карта — 32 строки по 32 символа: '#' — еда, '.' — клетка тропы без еды
 * (разрыв или пройденный поворот), пробел — пустое поле. Менять её нельзя:
 * результаты разных стратегий сравнимы только на одной и той же тропе, а эта
 * тропа — стандарт сравнения в литературе (89 клеток еды, 600 тактов).
 *
 * Карта перенесена из репозитория c_fsm (модуль clever_ant), где она задана
 * числами 0/1/2/3; здесь те же клетки записаны символами. Число клеток с едой
 * — 89 — проверяется тестом практики, а не принимается на веру.
 *
 * Старт — клетка (0, 0), направление — на восток.
 */

#include "ant_trail.h"

#include <string.h>

/* Тропа Санта-Фе. Стартовая клетка (0, 0) еды не содержит.
   Разметка карты по столбцам значима, поэтому она защищена от
   переформатирования. */
/* clang-format off */
static const char *const TRAIL[TRAIL_SIDE] = {
    ".##########                     ", /* 00 */
    "          #                     ", /* 01 */
    "          #                     ", /* 02 */
    "          #                     ", /* 03 */
    "          #   .#                ", /* 04 */
    "####      #   .         .#######", /* 05 */
    "   #      #   .         #       ", /* 06 */
    "   #      #   #         #       ", /* 07 */
    "   #      #.#..         #       ", /* 08 */
    "   #      #.            #       ", /* 09 */
    "   ########.            #       ", /* 10 */
    "           #      .#####.       ", /* 11 */
    "           ...#.  #             ", /* 12 */
    "               .  #             ", /* 13 */
    "               #  #             ", /* 14 */
    "           .#...  #             ", /* 15 */
    "           .      #             ", /* 16 */
    "           #      #             ", /* 17 */
    "       .#...      .             ", /* 18 */
    "       .          .             ", /* 19 */
    "       .          #             ", /* 20 */
    "       #          #             ", /* 21 */
    "    .#..          #             ", /* 22 */
    "    .             #             ", /* 23 */
    "    #             #             ", /* 24 */
    "    #             #             ", /* 25 */
    "    #             .             ", /* 26 */
    "    #..####.######.             ", /* 27 */
    "                                ", /* 28 */
    "                                ", /* 29 */
    "                                ", /* 30 */
    "                                "  /* 31 */
};
/* clang-format on */

/* Автомат, найденный эволюционным поиском: 17 состояний, 181 такт.
   Запись в формате журнала поиска, см. ant_fsm_parse(). */
static const char *const EVOLVED_SPEC
        = "3.0.0.1:3.13.0.2:3.0.0.3:3.14.0.12:3.2.3.7:3.2.2.13:3.9.2.16:3.8.0.11:"
          "3.7.1.15:3.10.3.2:3.12.3.13:3.5.1.6:3.6.2.0:3.7.2.6:3.14.2.4:3.13.0.12:"
          "3.9.1.15";

enum Direction { DIR_EAST, DIR_SOUTH, DIR_WEST, DIR_NORTH };

static int food_total(void);

void ant_walk_init(struct AntWalk *w) {
    int y;
    int x;

    memset(w, 0, sizeof(*w));
    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            w->food[y][x] = (char)(TRAIL[y][x] == '#');
        }
    }
    w->x = 0;
    w->y = 0;
    w->dir = DIR_EAST;
    w->state = 0;
    w->visited[0][0] = 1;
    w->run.total = food_total();
}

/* Поле замкнуто в тор: муравей, ушедший за край, появляется с другой
   стороны. Так задача не зависит от того, где нарисована граница. */
static void ahead(const struct AntWalk *w, int *nx, int *ny) {
    int dx = 0;
    int dy = 0;

    switch (w->dir) {
    case DIR_EAST:
        dx = 1;
        break;
    case DIR_SOUTH:
        dy = 1;
        break;
    case DIR_WEST:
        dx = -1;
        break;
    case DIR_NORTH:
        dy = -1;
        break;
    default:
        break;
    }
    *nx = (w->x + dx + TRAIL_SIDE) % TRAIL_SIDE;
    *ny = (w->y + dy + TRAIL_SIDE) % TRAIL_SIDE;
}

static bool food_ahead(const struct AntWalk *w) {
    int nx;
    int ny;

    ahead(w, &nx, &ny);
    return w->food[ny][nx] != 0;
}

static int food_total(void) {
    int y;
    int x;
    int count = 0;

    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            count += (TRAIL[y][x] == '#') ? 1 : 0;
        }
    }
    return count;
}

int ant_trail_food_total(void) {
    return food_total();
}

/*
 * Стратегия без слепых шагов: только осмотр вправо-влево. Её достаточно,
 * пока разрывы тропы не длиннее одной клетки; на первом же разрыве в две
 * клетки муравей встаёт. Автомат нужен для сравнения в лекции и тестах.
 */
void ant_fsm_lookaround(struct AntFsm *fsm) {
    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = 4;
    /* clang-format off */
    fsm->action[0][1] = ANT_STEP;       fsm->next[0][1] = 0;
    fsm->action[0][0] = ANT_TURN_RIGHT; fsm->next[0][0] = 1;
    fsm->action[1][1] = ANT_STEP;       fsm->next[1][1] = 0;
    fsm->action[1][0] = ANT_TURN_LEFT;  fsm->next[1][0] = 2;
    fsm->action[2][1] = ANT_STEP;       fsm->next[2][1] = 0;
    fsm->action[2][0] = ANT_TURN_LEFT;  fsm->next[2][0] = 3;
    fsm->action[3][1] = ANT_STEP;       fsm->next[3][1] = 0;
    fsm->action[3][0] = ANT_TURN_RIGHT; fsm->next[3][0] = 0;
    /* clang-format on */
}

void ant_fsm_probe(struct AntFsm *fsm) {
    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = 6;

    /*
     * Стратегия: пока еда впереди — идти по ней; когда еда кончилась —
     * обшарить окрестность поворотами вправо и влево, а если и это не помогло
     * — сделать два шага вслепую и начать осмотр заново.
     *
     * Состояния 1..5 и есть эта последовательность: каждое помнит, сколько
     * шагов поиска уже сделано. Как только еда найдена, автомат из любого
     * состояния возвращается в 0 — «иду по тропе».
     *
     * На тропе Санта-Фе такой стратегии не хватает: осмотр не заглядывает
     * назад, а два слепых шага проносят муравья мимо поворота. Съедает 59 из
     * 89 и зацикливается — см. тесты практики и лекцию 11.
     */
    /* clang-format off */
    /* 0: движение по тропе */
    fsm->action[0][1] = ANT_STEP;       fsm->next[0][1] = 0;
    fsm->action[0][0] = ANT_TURN_RIGHT; fsm->next[0][0] = 1;
    /* 1: посмотреть направо */
    fsm->action[1][1] = ANT_STEP;       fsm->next[1][1] = 0;
    fsm->action[1][0] = ANT_TURN_LEFT;  fsm->next[1][0] = 2;
    /* 2: вернуться на курс */
    fsm->action[2][1] = ANT_STEP;       fsm->next[2][1] = 0;
    fsm->action[2][0] = ANT_TURN_LEFT;  fsm->next[2][0] = 3;
    /* 3: посмотреть налево */
    fsm->action[3][1] = ANT_STEP;       fsm->next[3][1] = 0;
    fsm->action[3][0] = ANT_TURN_RIGHT; fsm->next[3][0] = 4;
    /* 4: разрыв длиннее одной клетки — шагнуть вперёд вслепую */
    fsm->action[4][1] = ANT_STEP;       fsm->next[4][1] = 0;
    fsm->action[4][0] = ANT_STEP;       fsm->next[4][0] = 5;
    /* 5: второй слепой шаг, затем осмотр начинается заново */
    fsm->action[5][1] = ANT_STEP;       fsm->next[5][1] = 0;
    fsm->action[5][0] = ANT_STEP;       fsm->next[5][0] = 0;
    /* clang-format on */
}

void ant_fsm_scan(struct AntFsm *fsm) {
    int i;

    memset(fsm, 0, sizeof(*fsm));
    fsm->state_count = 5;

    /*
     * Стратегия: еда впереди — шаг; иначе поворот направо. Четыре поворота
     * подряд осматривают все четыре направления и возвращают муравья на
     * прежний курс, после чего он делает один шаг вслепую.
     *
     * Отличие от ant_fsm_probe: осматриваются все направления, включая то,
     * откуда муравей пришёл, а вслепую делается один шаг, а не два. Этого
     * достаточно для всех разрывов тропы Санта-Фе — 89 из 89 за 315 тактов.
     */
    /* clang-format off */
    for (i = 0; i < 4; ++i) {
        fsm->action[i][1] = ANT_STEP;       fsm->next[i][1] = 0;
        fsm->action[i][0] = ANT_TURN_RIGHT; fsm->next[i][0] = i + 1;
    }
    /* 4: осмотр ничего не дал — шаг вслепую и осмотр заново */
    fsm->action[4][1] = ANT_STEP; fsm->next[4][1] = 0;
    fsm->action[4][0] = ANT_STEP; fsm->next[4][0] = 0;
    /* clang-format on */
}

void ant_fsm_evolved(struct AntFsm *fsm) {
    if (!ant_fsm_parse(fsm, EVOLVED_SPEC)) {
        memset(fsm, 0, sizeof(*fsm));
    }
}

bool ant_fsm_by_name(const char *name, struct AntFsm *fsm) {
    if (name == NULL) {
        return false;
    }
    if (strcmp(name, "lookaround") == 0) {
        ant_fsm_lookaround(fsm);
        return true;
    }
    if (strcmp(name, "probe") == 0) {
        ant_fsm_probe(fsm);
        return true;
    }
    if (strcmp(name, "scan") == 0) {
        ant_fsm_scan(fsm);
        return true;
    }
    if (strcmp(name, "evolved") == 0) {
        ant_fsm_evolved(fsm);
        return true;
    }
    return false;
}

/* --- текстовая запись автомата -------------------------------------------- */

/* Разбор целого без знака. Возвращает NULL, если цифр нет вовсе. */
static const char *parse_uint(const char *s, int *value) {
    int result = 0;
    int digits = 0;

    while (*s >= '0' && *s <= '9') {
        result = result * 10 + (*s - '0');
        if (result > ANT_FSM_MAX_STATES) {
            return NULL;
        }
        ++digits;
        ++s;
    }
    if (digits == 0) {
        return NULL;
    }
    *value = result;
    return s;
}

/* Разбор одного поля состояния: «действие . переход». */
static const char *parse_transition(const char *s, struct AntFsm *fsm, int state, int input) {
    int code;
    int next;

    s = parse_uint(s, &code);
    if (s == NULL || *s != '.') {
        return NULL;
    }
    s = parse_uint(s + 1, &next);
    if (s == NULL || next >= ANT_FSM_MAX_STATES) {
        return NULL;
    }
    switch (code) {
    case 0:
        fsm->action[state][input] = ANT_TURN_RIGHT;
        break;
    case 1:
        fsm->action[state][input] = ANT_TURN_LEFT;
        break;
    case 2:
    case 3:
        /* Шаг у нас съедает всегда, поэтому 2 и 3 неразличимы. */
        fsm->action[state][input] = ANT_STEP;
        break;
    default:
        return NULL;
    }
    fsm->next[state][input] = next;
    return s;
}

bool ant_fsm_parse(struct AntFsm *fsm, const char *spec) {
    int state = 0;
    int i;

    if (spec == NULL) {
        return false;
    }
    memset(fsm, 0, sizeof(*fsm));
    for (;;) {
        if (state >= ANT_FSM_MAX_STATES) {
            memset(fsm, 0, sizeof(*fsm));
            return false;
        }
        spec = parse_transition(spec, fsm, state, 1);
        if (spec == NULL || *spec != '.') {
            memset(fsm, 0, sizeof(*fsm));
            return false;
        }
        spec = parse_transition(spec + 1, fsm, state, 0);
        if (spec == NULL) {
            memset(fsm, 0, sizeof(*fsm));
            return false;
        }
        ++state;
        if (*spec != ':') {
            break;
        }
        ++spec;
    }
    if (*spec != '\0') {
        memset(fsm, 0, sizeof(*fsm));
        return false;
    }
    fsm->state_count = state;
    /* Переход в несуществующее состояние сделал бы прогон неопределённым. */
    for (i = 0; i < state; ++i) {
        if (fsm->next[i][0] >= state || fsm->next[i][1] >= state) {
            memset(fsm, 0, sizeof(*fsm));
            return false;
        }
    }
    return true;
}

/* Дозапись символа с проверкой ёмкости: snprintf в C90 нет. */
static bool put_char(char *out, size_t size, size_t *pos, char c) {
    if (*pos + 1 >= size) {
        return false;
    }
    out[*pos] = c;
    ++(*pos);
    return true;
}

static bool put_uint(char *out, size_t size, size_t *pos, int value) {
    char digits[8];
    int n = 0;

    if (value == 0) {
        return put_char(out, size, pos, '0');
    }
    while (value > 0 && n < (int)sizeof(digits)) {
        digits[n++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (n > 0) {
        if (!put_char(out, size, pos, digits[--n])) {
            return false;
        }
    }
    return true;
}

static bool put_transition(char *out, size_t size, size_t *pos, const struct AntFsm *fsm, int state,
                           int input) {
    int code;

    switch (fsm->action[state][input]) {
    case ANT_TURN_RIGHT:
        code = 0;
        break;
    case ANT_TURN_LEFT:
        code = 1;
        break;
    default:
        /* Шаг: в ветке «есть еда» он съедает клетку — код 3, иначе 2. */
        code = (input == 1) ? 3 : 2;
        break;
    }
    return put_uint(out, size, pos, code) && put_char(out, size, pos, '.')
           && put_uint(out, size, pos, fsm->next[state][input]);
}

bool ant_fsm_format(const struct AntFsm *fsm, char *out, size_t size) {
    size_t pos = 0;
    int i;

    if (out == NULL || size == 0 || fsm->state_count <= 0) {
        return false;
    }
    for (i = 0; i < fsm->state_count; ++i) {
        if (i > 0 && !put_char(out, size, &pos, ':')) {
            return false;
        }
        if (!put_transition(out, size, &pos, fsm, i, 1) || !put_char(out, size, &pos, '.')
            || !put_transition(out, size, &pos, fsm, i, 0)) {
            return false;
        }
    }
    out[pos] = '\0';
    return true;
}

/* --- прогон ---------------------------------------------------------------- */

bool ant_walk_step(struct AntWalk *w, const struct AntFsm *fsm) {
    int input;
    enum AntAction action;

    if (fsm->state_count <= 0 || w->run.finished) {
        return false;
    }
    input = food_ahead(w) ? 1 : 0;
    action = fsm->action[w->state][input];
    switch (action) {
    case ANT_STEP: {
        int nx;
        int ny;
        ahead(w, &nx, &ny);
        w->x = nx;
        w->y = ny;
        w->visited[ny][nx] = 1;
        if (w->food[ny][nx]) {
            w->food[ny][nx] = 0;
            ++w->run.eaten;
            w->run.last_eaten = w->run.steps + 1;
        }
        break;
    }
    case ANT_TURN_LEFT:
        w->dir = (w->dir + 3) % 4;
        break;
    case ANT_TURN_RIGHT:
        w->dir = (w->dir + 1) % 4;
        break;
    default:
        break;
    }
    w->state = fsm->next[w->state][input];
    ++w->run.steps;
    if (w->run.eaten == w->run.total) {
        w->run.finished = true;
    }
    return true;
}

static struct TrailRun run(const struct AntFsm *fsm, int steps_limit, struct AntWalk *w) {
    ant_walk_init(w);
    while (w->run.steps < steps_limit && ant_walk_step(w, fsm)) {}
    return w->run;
}

struct TrailRun ant_trail_run(const struct AntFsm *fsm, int steps_limit) {
    struct AntWalk w;
    return run(fsm, steps_limit, &w);
}

bool ant_trail_on_path(int x, int y) {
    if (x < 0 || x >= TRAIL_SIDE || y < 0 || y >= TRAIL_SIDE) {
        return false;
    }
    return TRAIL[y][x] != ' ';
}

bool ant_trail_has_food(int x, int y) {
    if (x < 0 || x >= TRAIL_SIDE || y < 0 || y >= TRAIL_SIDE) {
        return false;
    }
    return TRAIL[y][x] == '#';
}

void ant_trail_snapshot(const struct AntFsm *fsm, int steps_limit, struct TrailSnapshot *out) {
    struct AntWalk w;
    int y;
    int x;

    out->run = run(fsm, steps_limit, &w);
    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            out->food[y][x] = w.food[y][x];
            out->visited[y][x] = w.visited[y][x];
        }
    }
    out->x = w.x;
    out->y = w.y;
    out->dir = w.dir;
}

void ant_trail_print(const struct AntFsm *fsm, int steps_limit, FILE *out) {
    struct AntWalk w;
    struct TrailRun r = run(fsm, steps_limit, &w);
    int y;
    int x;

    fprintf(out, "Съедено %d из %d за %d тактов%s\n\n", r.eaten, r.total, r.steps,
            r.finished ? " (тропа пройдена)" : "");
    for (y = 0; y < TRAIL_SIDE; ++y) {
        for (x = 0; x < TRAIL_SIDE; ++x) {
            if (x == w.x && y == w.y) {
                fputc('@', out);
            } else if (w.food[y][x]) {
                fputc('#', out);
            } else if (TRAIL[y][x] == '#') {
                fputc('o', out);
            } else if (TRAIL[y][x] == '.') {
                fputc('.', out);
            } else {
                fputc(' ', out);
            }
        }
        fputc('\n', out);
    }
}
