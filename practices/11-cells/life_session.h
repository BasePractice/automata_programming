#ifndef STATECRAFT_LIFE_SESSION_H
#define STATECRAFT_LIFE_SESSION_H

/**
 * @file
 * Лекция 11. Сеанс игры «Жизнь» для терминальной программы life_tui: поле,
 * правило, номер поколения, история для шага назад и каталог готовых
 * объектов.
 *
 * Модуль не знает ни о терминале, ни о клавишах: программа только рисует
 * поле и переводит нажатия в вызовы отсюда. Поэтому всё, что программа
 * умеет, проверяется тестом, а не глазами, — то же требование, что у
 * графики практики 21 и модели установки практики 8.
 *
 * Шаг назад в «Жизни» нельзя вычислить: у конфигурации бывает много
 * предшественников, а у «Сада Эдема» нет ни одного. Поэтому назад сеанс не
 * считает, а вспоминает — хранит последние поколения в кольцевом буфере.
 */

#include "base_types.h"
#include "cells.h"

#if defined(__cplusplus)
extern "C" {
#endif

/** Правило перехода: «Жизнь» Конуэя или правило чётности Фредкина. */
enum LifeRule {
    LIFE_RULE_CONWAY, /**< B3/S23, окрестность Мура */
    LIFE_RULE_PARITY  /**< нечётное число соседей фон Неймана */
};

/** Сколько поколений помнит сеанс для шага назад. */
#define LIFE_SESSION_HISTORY 128

/** Поле, правило и история; заводится life_session_create. */
struct LifeSession {
    struct Life life;
    enum LifeRule rule;
    long generation;      /**< номер поколения от начальной расстановки */
    struct Life *history; /**< кольцо из #LIFE_SESSION_HISTORY полей */
    int history_head;     /**< куда ляжет следующее сохранённое поколение */
    int history_count;    /**< сколько поколений можно вернуть */
};

/**
 * Пустое поле @p width x @p height (приводится к 1..#LIFE_MAX_SIDE), правило
 * Конуэя.
 *
 * @return NULL — не хватило памяти. Сеанс занимает около 8 МБ, поэтому
 *         живёт в куче; освобождается life_session_destroy.
 */
struct LifeSession *life_session_create(int width, int height);

void life_session_destroy(struct LifeSession *session);

/**
 * Новый размер поля. Содержимое остаётся по центру: при уменьшении края
 * срезаются, при увеличении добавляются пустые. История сбрасывается — ход
 * назад на поле другого размера был бы ходом другого автомата.
 */
void life_session_resize(struct LifeSession *session, int width, int height);

/**
 * Случайная расстановка: каждая клетка жива с вероятностью @p percent
 * процентов. Числа берутся из rand(), затравку задаёт вызывающий.
 * Правило становится Конуэя, счётчик поколений — нулём.
 */
void life_session_randomize(struct LifeSession *session, int percent);

/** Ход вперёд; текущее поле запоминается для шага назад. */
void life_session_forward(struct LifeSession *session);

/**
 * Ход назад — возврат к запомненному поколению.
 *
 * @return false — помнить нечего: это начальная расстановка либо история
 *         исчерпана. Поле при этом не меняется.
 */
bool life_session_back(struct LifeSession *session);

/* --- каталог готовых объектов -------------------------------------------- */

/** Готовый объект: фигура или сцена из нескольких фигур. */
struct LifePreset {
    const char *title;   /**< название для меню */
    const char *group;   /**< раздел меню: откуда объект и что показывает */
    const char *note;    /**< одна строка о том, на что смотреть */
    const char *pattern; /**< имя фигуры для life_place или NULL у сцены */
    int scene;           /**< номер сцены, если pattern == NULL */
    enum LifeRule rule;
};

/** Объект каталога по номеру от нуля; NULL — номера кончились. */
const struct LifePreset *life_preset(int index);

int life_preset_count(void);

/**
 * Наименьшее поле, на котором объект виден целиком и не задевает сам себя
 * через край тора. Программа уменьшает клетку, пока поле не станет не
 * меньше этого.
 */
void life_preset_min_size(const struct LifePreset *preset, int *width, int *height);

/**
 * Очистить поле и поставить объект в центр; правило берётся из объекта,
 * счётчик поколений и история сбрасываются.
 *
 * Сцены с агаром подрезают поле до кратного трём: на шве тора агар
 * разрушился бы сам, без всякого вируса (см. life_fill_agar).
 *
 * @return false — объект не поставлен (неизвестная фигура); поле пустое.
 */
bool life_session_load(struct LifeSession *session, const struct LifePreset *preset);

#if defined(__cplusplus)
}
#endif

#endif /* STATECRAFT_LIFE_SESSION_H */
