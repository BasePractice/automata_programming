#ifndef STATECRAFT_ANT_SESSION_H
#define STATECRAFT_ANT_SESSION_H

/**
 * @file
 * Лекция 11. Сеанс терминальной программы ant_tui: прогон автомата муравья
 * по тропе Санта-Фе такт за тактом, каталог готовых автоматов, режимы
 * поиска и сравнение автомата с предыдущим.
 *
 * Как и у «Жизни» (life_session.h), модуль не знает о терминале: программа
 * только рисует и переводит нажатия в вызовы отсюда, а всё, что она умеет,
 * проверяется тестом.
 *
 * Мнемоника автомата — запись перехода тремя знаками: что сделать и куда
 * перейти. «Ш» — шаг вперёд, «Л» — поворот налево, «П» — направо; номер —
 * следующее состояние. У каждого состояния два перехода: при еде впереди
 * (●) и без неё (○). Так «q0 ●Ш0 ○П1» читается: «в состоянии 0, видя еду,
 * шагни и останься в 0; не видя — повернись направо и перейди в 1».
 */

#include "ant_random.h"
#include "ant_search.h"
#include "ant_trail.h"
#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Длина названия показываемого автомата, с завершающим нулём. */
#define ANT_SESSION_TITLE_SIZE 128

/** Автомат на тропе и все такты его прогона — для шага назад. */
struct AntSession {
    struct AntFsm fsm;      /**< показываемый автомат */
    struct AntFsm previous; /**< показанный до него: от него считаются изменения */
    int has_previous;       /**< 0 — сравнивать не с чем (int, а не bool: см. base_types.h) */
    int steps_limit;        /**< лимит тактов прогона */
    char title[ANT_SESSION_TITLE_SIZE];
    struct AntWalk *history; /**< history[0] — старт, history[count - 1] — текущий такт */
    int count;
};

/** Пустой сеанс: автомата нет, тропа нетронута. NULL — нет памяти; сеанс
    держит все такты прогона (около 1,3 МБ) и живёт в куче. */
struct AntSession *ant_session_create(void);

void ant_session_destroy(struct AntSession *s);

/**
 * Показать автомат: прогон начинается заново с такта 0. Прежний автомат
 * становится предыдущим — по нему подсвечиваются изменения. Лимит тактов
 * @p steps_limit приводится к 1..#TRAIL_STEPS.
 */
void ant_session_show(struct AntSession *s, const struct AntFsm *fsm, int steps_limit,
                      const char *title);

/** Забыть предыдущий автомат: подсвечивать больше нечего. */
void ant_session_forget_previous(struct AntSession *s);

/** Текущий такт прогона. */
const struct AntWalk *ant_session_walk(const struct AntSession *s);

/** Такт вперёд. false — прогон окончен: лимит, вся еда съедена или у
    автомата нет состояний. */
bool ant_session_forward(struct AntSession *s);

/** Такт назад. false — это начало прогона. */
bool ant_session_back(struct AntSession *s);

/** Прогнать до конца и вернуться к началу. */
void ant_session_to_end(struct AntSession *s);
void ant_session_to_start(struct AntSession *s);

/** Окончен ли прогон: дальше шагать некуда. */
bool ant_session_done(const struct AntSession *s);

/** Случайный автомат из @p states состояний (1..#ANT_FSM_MAX_STATES). */
void ant_fsm_random(struct AntFsm *fsm, int states, struct AntRandom *r);

/* --- мнемоника и сравнение ------------------------------------------------ */

/** Что изменилось в переходе по сравнению с предыдущим автоматом. */
enum AntDiff {
    ANT_DIFF_SAME = 0,   /**< переход тот же */
    ANT_DIFF_ACTION = 1, /**< другое действие */
    ANT_DIFF_NEXT = 2,   /**< другое следующее состояние */
    ANT_DIFF_NEW = 4     /**< у предыдущего автомата такого состояния не было */
};

/** Изменения перехода (@p state, @p input) в @p current относительно
    @p previous: сочетание флагов #AntDiff. */
int ant_fsm_diff(const struct AntFsm *previous, const struct AntFsm *current, int state, int input);

/** Сколько переходов изменилось — та «длина шага» эволюции, которую видно
    на экране. Новые состояния считаются целиком. */
int ant_fsm_diff_count(const struct AntFsm *previous, const struct AntFsm *current);

/** Мнемоника перехода: «Ш3», «Л12». @p out — не меньше 8 байт. */
void ant_transition_text(const struct AntFsm *fsm, int state, int input, char *out);

/* --- каталог готовых автоматов -------------------------------------------- */

/** Готовый автомат и то, что он показывает на тропе. */
struct AntPreset {
    const char *title; /**< название для меню */
    const char *group; /**< откуда автомат */
    const char *note;  /**< на что смотреть */
    const char *spec;  /**< имя стратегии ant_fsm_by_name или запись журнала */
    int steps_limit;   /**< лимит тактов, с которым автомат показывается */
    int eaten;         /**< сколько еды съедает за лимит */
    int steps;         /**< за сколько тактов (лимит, если не доедает) */
};

const struct AntPreset *ant_preset(int index);
int ant_preset_count(void);

/** Таблица автомата из каталога. false — запись испорчена (тест это
    исключает). */
bool ant_preset_fsm(const struct AntPreset *preset, struct AntFsm *fsm);

/* --- режимы поиска --------------------------------------------------------- */

/** Откуда поиск начинает. */
enum AntSearchStart {
    ANT_START_RANDOM, /**< со случайных автоматов */
    ANT_START_SCAN,   /**< со стратегии scan (5 состояний, 315 тактов) */
    ANT_START_CURRENT /**< с автомата, который сейчас на экране */
};

/** Режим поиска для меню программы. */
struct AntSearchMode {
    const char *title;
    const char *note;
    enum AntSearchAlgorithm algorithm;
    enum AntSearchStart start;
    int max_states;
    long stall_limit;
    long iterations;
    int compression; /**< EP93: заморозка модулей (int, а не bool: см. base_types.h) */
};

const struct AntSearchMode *ant_search_mode(int index);
int ant_search_mode_count(void);

/**
 * Настройки поиска по режиму. Для #ANT_START_CURRENT потолок состояний
 * поднимается до размера @p current, если тот больше. Начальный автомат
 * записывается в @p start; @p has_start = 0 — поиск начнётся со случайных
 * автоматов.
 */
void ant_search_mode_config(const struct AntSearchMode *mode, uint32_t seed,
                            const struct AntFsm *current, struct AntSearch *cfg,
                            struct AntFsm *start, int *has_start);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ANT_SESSION_H */
