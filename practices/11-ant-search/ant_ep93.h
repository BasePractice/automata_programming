#ifndef STATECRAFT_ANT_EP93_H
#define STATECRAFT_ANT_EP93_H

/**
 * @file
 * Внутренний заголовок практики: эволюционное программирование по статье
 * Angeline P. J., Pollack J. B. «Evolutionary Module Acquisition» (Proc. of
 * the 2nd Annual Conference on Evolutionary Programming, 1993).
 *
 * Наружу выходит только ant_search_run() из ant_search.h, которая выбирает
 * алгоритм по настройкам; отдельный заголовок нужен, чтобы популяционный
 * алгоритм лежал в своём файле и не мешался с простым (1 + 1)-поиском.
 */

#include "ant_search.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Состояние поиска по алгоритму статьи между вызовами; внутреннее. */
struct Ep93State;

/** Начало поиска: популяция заведена и оценена. NULL — настройки
    бессмысленны (популяция меньше двух, нет состязаний) или нет памяти. */
struct Ep93State *ant_ep93_begin(const struct AntSearch *cfg, const struct AntFsm *start,
                                 const struct AntSearchLog *log);

/** Продолжить поиск примерно на @p budget оценок (поколение не делится).
    true — поиск окончен: тропа пройдена или бюджет настроек исчерпан. */
bool ant_ep93_advance(struct Ep93State *s, long budget, const struct AntSearchLog *log);

const struct AntSearchResult *ant_ep93_result(const struct Ep93State *s);

void ant_ep93_end(struct Ep93State *s);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_ANT_EP93_H */
