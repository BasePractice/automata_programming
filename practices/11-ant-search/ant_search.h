#ifndef STATECRAFT_ANT_SEARCH_H
#define STATECRAFT_ANT_SEARCH_H

/**
 * @file
 * Лекция 11, продолжение задачи об умном муравье: автомат не строится по
 * описанию поведения, а ищется.
 *
 * Практика 11-cells даёт постановку — тропу Санта-Фе, прогон автомата и
 * несколько стратегий, придуманных вручную. Здесь та же тропа служит функцией
 * приспособленности: автомат — это таблица переходов, качество автомата —
 * пара «сколько съедено, за сколько тактов».
 *
 * Алгоритмов поиска два, и выбираются они параметром:
 *
 *   ANT_SEARCH_HILL  — (1 + 1)-эволюционная стратегия: один автомат, точечные
 *                      мутации, принимается не худшая копия. Простейший из
 *                      работающих методов; годится, чтобы увидеть сам приём;
 *   ANT_SEARCH_EP93  — эволюционное программирование по статье Angeline и
 *                      Pollack «Evolutionary Module Acquisition» (1993):
 *                      популяция, состязательный отбор, мутации состояний и
 *                      переходов, заморозка модулей. Ровно та постановка, на
 *                      которой авторы проверяли свою идею, — та же тропа, тот
 *                      же лимит 200 тактов, тот же потолок в 32 состояния.
 *
 * Почему не полный перебор: у автомата с n состояниями и двумя входными
 * символами (3n)^(2n) таблиц — при n = 5 это около 5.8 * 10^11.
 *
 * Никакого глобального состояния: датчик случайных чисел живёт в структуре
 * поиска, поэтому два прогона с одним зерном дают побайтово один результат, а
 * несколько поисков в одном процессе не мешают друг другу.
 */

#include "ant_trail.h"
#include "base_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Каким методом искать автомат. */
enum AntSearchAlgorithm {
    ANT_SEARCH_HILL, /**< (1 + 1)-эволюционная стратегия */
    ANT_SEARCH_EP93  /**< эволюционное программирование, Angeline и Pollack, 1993 */
};

/** Настройки поиска. Готовые наборы — #ant_search_defaults и #ant_search_defaults_ep93. */
struct AntSearch {
    enum AntSearchAlgorithm algorithm;
    int max_states;   /**< потолок числа состояний, не больше #ANT_FSM_MAX_STATES */
    int steps_limit;  /**< тактов на прогон: #TRAIL_STEPS или #TRAIL_STEPS_EP93 */
    long iterations;  /**< бюджет: мутаций для HILL, оценённых автоматов для EP93 */
    long stall_limit; /**< HILL: столько неудач подряд — и добавляется состояние; 0 — не расти */
    uint32_t seed;    /**< зерно датчика: один и тот же поиск воспроизводится */

    int population;   /**< EP93: размер популяции (в статье 300) */
    int tournaments;  /**< EP93: состязаний на особь при отборе (в статье 5) */
    bool compression; /**< EP93: заморозка модулей — главный предмет статьи */
};

/** Что дал поиск. */
struct AntSearchResult {
    struct AntFsm best;   /**< лучший найденный автомат */
    struct TrailRun run;  /**< его прогон по тропе */
    long improvements;    /**< сколько раз результат улучшался */
    long iterations_done; /**< сколько итераций сделано (HILL) */
    long evaluations;     /**< сколько автоматов прогнано по тропе */
    /** На какой по счёту оценке тропа впервые пройдена целиком (0 — не
        пройдена). Это и есть мера, по которой статья сравнивает методы. */
    long solved_at;
    long generations;  /**< сколько поколений сменилось (EP93) */
    long compressions; /**< сколько раз замораживались части автоматов (EP93) */
};

/**
 * Журнал улучшений: вызывается каждый раз, когда найден автомат лучше
 * прежнего. Ровно из таких записей состоит история поиска в лекции
 * (315 → 295 → … → 181 такт). Поле callback может быть NULL.
 */
struct AntSearchLog {
    void (*improved)(const struct AntFsm *fsm, const struct TrailRun *run, long iteration,
                     void *user);
    void *user;
};

/** Настройки по умолчанию: (1 + 1)-ЭС, 5 состояний, лимит #TRAIL_STEPS,
    200 000 итераций, рост при застое выключен, зерно 1. Поля EP93 при этом
    заполняются значениями из статьи: популяция 300, 5 состязаний, заморозка
    включена. */
void ant_search_defaults(struct AntSearch *cfg);

/** Настройки постановки статьи: алгоритм EP93, 32 состояния, лимит 200 тактов,
    бюджет в 1 000 000 оценённых автоматов. Зерно не меняется. */
void ant_search_defaults_ep93(struct AntSearch *cfg);

/**
 * Поиск. @p start — автомат, с которого начинать (NULL — случайные автоматы).
 * Возвращает false, если настройки бессмысленны (нулевой лимит тактов,
 * состояний больше ANT_FSM_MAX_STATES и тому подобное); в этом случае out не
 * меняется. log может быть NULL.
 */
bool ant_search_run(const struct AntSearch *cfg, const struct AntFsm *start,
                    struct AntSearchResult *out, const struct AntSearchLog *log);

/** Поиск, идущий по частям; внутреннее устройство скрыто. */
struct AntSearchState;

/**
 * Начать поиск, не делая ни одной итерации сверх начальной оценки. Нужен
 * тому, кто ищет по частям и между ними что-то показывает (программа
 * ant_tui). Деление на части результата не меняет: ant_search_run устроен
 * как begin + advance без предела + end.
 *
 * @return NULL — настройки бессмысленны (как у ant_search_run) или нет
 *         памяти. Освобождается ant_search_end.
 */
struct AntSearchState *ant_search_begin(const struct AntSearch *cfg, const struct AntFsm *start,
                                        const struct AntSearchLog *log);

/**
 * Продолжить поиск примерно на @p budget оценок: у EP93 поколение не
 * делится, поэтому бюджет может быть превышен на половину популяции.
 *
 * @return true — поиск окончен: бюджет настроек исчерпан либо (EP93) тропа
 *         пройдена. Дальнейшие вызовы ничего не делают.
 */
bool ant_search_advance(struct AntSearchState *s, long budget, const struct AntSearchLog *log);

/** Итог на текущий момент: лучший автомат и счётчики. */
const struct AntSearchResult *ant_search_result(const struct AntSearchState *s);

/** Настройки, с которыми поиск начат. */
const struct AntSearch *ant_search_config(const struct AntSearchState *s);

void ant_search_end(struct AntSearchState *s);

/**
 * Сравнение качества прогонов: сначала по числу съеденных клеток, при
 * равенстве — по числу тактов. Возвращает true, если a лучше b. Вынесено в
 * заголовок, потому что это и есть функция приспособленности задачи, и в
 * тестах она проверяется отдельно от поиска.
 */
bool ant_run_better(const struct TrailRun *a, const struct TrailRun *b);

/**
 * Приспособленность по формуле (4) статьи Angeline и Pollack:
 *
 *     food + 0.01 * (1 - t / limit),
 *
 * где food — число съеденных клеток, t — такт, на котором съедена последняя.
 * Дробная добавка меньше единицы, поэтому автомат, съевший больше, всегда
 * лучше автомата, съевшего меньше; при равной еде выигрывает тот, кто
 * управился быстрее.
 */
double ant_fitness_ep93(const struct TrailRun *run, int steps_limit);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ANT_SEARCH_H */
